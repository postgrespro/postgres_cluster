/*
 * raftable.c
 *
 * A key-value table replicated over Raft.
 *
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "storage/ipc.h"
#include "access/htup_details.h"
#include "miscadmin.h"
#include "funcapi.h"
#include "utils/timestamp.h"

#include "raft.h"
#include "util.h"

#include "raftable.h"
#include "worker.h"
#include "state.h"

#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <time.h>

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(raftable_sql_get);
PG_FUNCTION_INFO_V1(raftable_sql_set);
PG_FUNCTION_INFO_V1(raftable_sql_list);

static struct {
	void *state;
	int *leader;
} shared;

static int leadersock = -1;
static WorkerConfig wcfg;
static char *peerstr;
static shmem_startup_hook_type PreviousShmemStartupHook;

static void *get_shared_state(void)
{
	return shared.state;
}

static void select_next_peer(void)
{
	int orig_leader = *shared.leader;
	int i;
	for (i = 0; i < RAFTABLE_PEERS_MAX; i++)
	{
		int idx = (orig_leader + i + 1) % RAFTABLE_PEERS_MAX;
		HostPort *hp = wcfg.peers + idx;
		if (hp->up)
		{
			*shared.leader = idx;
			return;
		}
	}
	elog(WARNING, "all raftable peers down");
}

static void disconnect_leader(void)
{
	if (leadersock >= 0)
	{
		close(leadersock);
	}
	select_next_peer();
	leadersock = -1;
}

static bool poll_until_writable(int sock, int timeout_ms)
{
	struct pollfd pfd = {sock, POLLOUT, 0};
	int r = poll(&pfd, 1, timeout_ms);
	if (r != 1) return false;
	return pfd.revents & POLLOUT;
}

static bool poll_until_readable(int sock, int timeout_ms)
{
	struct pollfd pfd = {sock, POLLIN, 0};
	int r = poll(&pfd, 1, timeout_ms);
	if (r != 1) return false;
	return pfd.revents & POLLIN;
}

static long msec(TimestampTz timer)
{
	long sec;
	int usec;
	TimestampDifference(0, timer, &sec, &usec);
	return sec * 1000 + usec / 1000;
}

static bool timed_write(int sock, void *data, size_t len, int timeout_ms)
{
	TimestampTz start, now;
	int sent = 0;

	now = start = GetCurrentTimestamp();

	while (sent < len)
	{
		int newbytes;
		now = GetCurrentTimestamp();
		if ((timeout_ms != -1) && (msec(now - start) > timeout_ms)) {
			elog(WARNING, "write timed out");
			return false;
		}

		newbytes = write(sock, (char *)data + sent, len - sent);
		if (newbytes == -1)
		{
			if (errno == EAGAIN) {
				if (poll_until_writable(sock, timeout_ms - msec(now - start))) {
					continue;
				}
			}
			elog(WARNING, "failed to write: %s", strerror(errno));
			return false;
		}
		sent += newbytes;
	}

	return true;
}

static bool timed_read(int sock, void *data, size_t len, int timeout_ms)
{
	int recved = 0;
	TimestampTz start, now;
	now = start = GetCurrentTimestamp();

	while (recved < len)
	{
		int newbytes;
		now = GetCurrentTimestamp();
		if ((timeout_ms != -1) && (msec(now - start) > timeout_ms)) {
			elog(WARNING, "read timed out");
			return false;
		}

		newbytes = read(sock, (char *)data + recved, len - recved);
		if (newbytes == -1)
		{
			if (errno == EAGAIN) {
				if (poll_until_readable(sock, timeout_ms - msec(now - start))) {
					continue;
				}
			}
			elog(WARNING, "failed to read: %s", strerror(errno));
			return false;
		}
		recved += newbytes;
	}

	return true;
}

static bool connect_leader(int timeout_ms)
{
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;
	int rc;

	TimestampTz now;
	int elapsed_ms;

	HostPort *leaderhp;

	if (*shared.leader == NOBODY) select_next_peer();

	leaderhp = wcfg.peers + *shared.leader;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", leaderhp->port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;

	if ((rc = getaddrinfo(leaderhp->host, portstr, &hint, &addrs)))
	{
		disconnect_leader();
		fprintf(stderr, "failed to resolve address '%s:%d': %s",
				leaderhp->host, leaderhp->port,
				gai_strerror(rc));
		return false;
	}

	fprintf(stderr, "trying [%d] %s:%d\n", *shared.leader, leaderhp->host, leaderhp->port);
	elapsed_ms = 0;
	now = GetCurrentTimestamp();
	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int one = 1;

		int sd = socket(a->ai_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (sd == -1)
		{
			perror("failed to create a socket");
			continue;
		}
		setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

		if (connect(sd, a->ai_addr, a->ai_addrlen) == -1)
		{
			if (errno == EINPROGRESS)
			{
				while ((elapsed_ms <= timeout_ms) || (timeout_ms == -1))
				{
					TimestampTz past = now;

					if (poll_until_writable(sd, timeout_ms - elapsed_ms))
					{
						int err;
						socklen_t optlen = sizeof(err);
						getsockopt(sd, SOL_SOCKET, SO_ERROR, &err, &optlen);
						if (err == 0)
						{
							// success
							break;
						}
					}

					now = GetCurrentTimestamp();
					elapsed_ms += msec(now - past);
				}
			}
			else
			{
				perror("failed to connect to an address");
				close(sd);
				continue;
			}
		}

		/* success */
		freeaddrinfo(addrs);
		leadersock = sd;
		return true;
	}
	freeaddrinfo(addrs);
	disconnect_leader();
	fprintf(stderr, "could not connect\n");
	return false;
}

static int get_connection(int timeout_ms)
{
	if (leadersock < 0)
	{
		if (connect_leader(timeout_ms)) return leadersock;
//		int timeout_ms = 100;
//		struct timespec timeout = {0, timeout_ms * 1000000};
//		nanosleep(&timeout, NULL);
	}
	return leadersock;
}

char *raftable_get(const char *key, size_t *len)
{
	Assert(wcfg.id >= 0);
	return state_get(shared.state, key, len);
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	size_t len;
	char *s;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	Assert(shared.state);

	s = state_get(shared.state, key.data, &len);
	if (s)
	{
		text *t = cstring_to_text_with_len(s, len);
		pfree(s);
		PG_RETURN_TEXT_P(t);
	}
	else
		PG_RETURN_NULL();
}

static bool try_sending_update(RaftableUpdate *ru, size_t size, int timeout_ms)
{
	int s, status;
	TimestampTz start, now;

	now = start = GetCurrentTimestamp();

	s = get_connection(timeout_ms - (now - start));
	if (s < 0) return false;

	now = GetCurrentTimestamp();
	if ((timeout_ms != -1) && (msec(now - start) > timeout_ms))
	{
		elog(WARNING, "update: connect() timed out");
		return false;
	}

	if (!timed_write(s, &size, sizeof(size), timeout_ms - msec(now - start)))
	{
		elog(WARNING, "failed to send the update size to the leader");
		return false;
	}

	now = GetCurrentTimestamp();
	if ((timeout_ms != -1) && (msec(now - start) > timeout_ms))
	{
		elog(WARNING, "update: send(size) timed out");
		return false;
	}

	if (!timed_write(s, ru, size, timeout_ms - msec(now - start)))
	{
		elog(WARNING, "failed to send the update to the leader");
		return false;
	}

	now = GetCurrentTimestamp();
	if ((timeout_ms != -1) && (msec(now - start) > timeout_ms))
	{
		elog(WARNING, "update: send(body) timed out");
		return false;
	}

	if (!timed_read(s, &status, sizeof(status), timeout_ms - msec(now - start)))
	{
		elog(WARNING, "failed to recv the update status from the leader");
		return false;
	}

	now = GetCurrentTimestamp();
	if ((timeout_ms != -1) && (msec(now - start) > timeout_ms))
	{
		elog(WARNING, "update: recv(status) timed out");
		return false;
	}

	if (status != 1)
	{
		elog(WARNING, "update: leader returned status = %d", status);
		return false;
	}

	return true;
}

bool raftable_set(const char *key, const char *value, size_t vallen, int timeout_ms)
{
	RaftableField *f;
	RaftableUpdate *ru;
	size_t size = sizeof(RaftableUpdate);
	size_t keylen = 0;
	TimestampTz now;
	int elapsed_ms;

	Assert(wcfg.id >= 0);

	keylen = strlen(key) + 1;

	size += sizeof(RaftableField) - 1;
	size += keylen;
	size += vallen;
	ru = palloc(size);

	ru->expector = wcfg.id;
	ru->fieldnum = 1;

	f = (RaftableField *)ru->data;
	f->keylen = keylen;
	f->vallen = vallen;
	memcpy(f->data, key, keylen);
	memcpy(f->data + keylen, value, vallen);

	elapsed_ms = 0;
	now = GetCurrentTimestamp();
	while ((elapsed_ms <= timeout_ms) || (timeout_ms == -1))
	{
		TimestampTz past = now;
		if (try_sending_update(ru, size, timeout_ms - elapsed_ms))
		{
			pfree(ru);
			return true;
		}
		else
		{
			disconnect_leader();
		}
		now = GetCurrentTimestamp();
		elapsed_ms += msec(now - past);
	}

	pfree(ru);
	elog(WARNING, "failed to set raftable value after %d ms", elapsed_ms);
	return false;
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	char *key = text_to_cstring(PG_GETARG_TEXT_P(0));
	int timeout_ms = PG_GETARG_INT32(2);
	if (PG_ARGISNULL(1))
		raftable_set(key, NULL, 0, timeout_ms);
	else
	{
		char *value = text_to_cstring(PG_GETARG_TEXT_P(1));
		raftable_set(key, value, strlen(value), timeout_ms);
		pfree(value);
	}
	pfree(key);

	PG_RETURN_VOID();
}

void raftable_every(void (*func)(const char *, const char *, size_t, void *), void *arg)
{
	void *scan;
	char *key, *value;
	size_t len;
	Assert(shared.state);
	Assert(wcfg.id >= 0);

	scan = state_scan(shared.state);
	while (state_next(shared.state, scan, &key, &value, &len))
	{
		func(key, value, len, arg);
		pfree(key);
		pfree(value);
	}
}

Datum
raftable_sql_list(PG_FUNCTION_ARGS)
{
	char *key, *value;
	size_t len;
	FuncCallContext *funcctx;
	MemoryContext oldcontext;

	Assert(shared.state);
	Assert(wcfg.id >= 0);

	if (SRF_IS_FIRSTCALL())
	{
		TypeFuncClass tfc;
		funcctx = SRF_FIRSTCALL_INIT();

		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		tfc = get_call_result_type(fcinfo, NULL, &funcctx->tuple_desc);
		if (tfc != TYPEFUNC_COMPOSITE)
		{
			elog(ERROR, "raftable listing function should be composite");
		}
		funcctx->tuple_desc = BlessTupleDesc(funcctx->tuple_desc);

		funcctx->user_fctx = state_scan(shared.state);
		Assert(funcctx->user_fctx);

		MemoryContextSwitchTo(oldcontext);

	}

	funcctx = SRF_PERCALL_SETUP();

	if (state_next(shared.state, funcctx->user_fctx, &key, &value, &len))
	{
		HeapTuple tuple;
		Datum  vals[2];
		bool isnull[2];

		vals[0] = PointerGetDatum(cstring_to_text(key));
		vals[1] = PointerGetDatum(cstring_to_text_with_len(value, len));
		isnull[0] = isnull[1] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc, vals, isnull);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		SRF_RETURN_DONE(funcctx);
	}
}

static void request_shmem(void)
{
	RequestAddinShmemSpace(sizeof(int)); /* for 'leader' id */
	state_shmem_request();
}

static void startup_shmem(void)
{
	bool found;

	if (PreviousShmemStartupHook) PreviousShmemStartupHook();

	shared.state = state_shmem_init();
	shared.leader = ShmemInitStruct("raftable_leader", sizeof(int), &found);
	*shared.leader = NOBODY;
}

void
_PG_init(void)
{
	wcfg.getter = get_shared_state;

	if (!process_shared_preload_libraries_in_progress)
		elog(ERROR, "please add 'raftable' to shared_preload_libraries list");

	wcfg.raft_config.peernum_max = RAFTABLE_PEERS_MAX;

	DefineCustomIntVariable("raftable.id",
		"Raft peer id of current instance", NULL,
		&wcfg.id, -1,
		-1, RAFTABLE_PEERS_MAX-1,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.heartbeat_ms",
		"Raft heartbeat timeout in ms", NULL,
		&wcfg.raft_config.heartbeat_ms, 20,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.election_ms_min",
		"Raft min election timeout in ms", NULL,
		&wcfg.raft_config.election_ms_min, 150,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.election_ms_max",
		"Raft max election timeout in ms", NULL,
		&wcfg.raft_config.election_ms_max, 300,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.log_len",
		"Raft log length", NULL,
		&wcfg.raft_config.log_len, 512,
		8, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.chunk_len",
		"Raft chunk length", NULL,
		&wcfg.raft_config.chunk_len, 400,
		1, 400,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.msg_len_max",
		"Raft chunk length", NULL,
		&wcfg.raft_config.msg_len_max, 500,
		1, 500,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomStringVariable("raftable.peers",
		"Raft peer list",
		"A comma separated list of id:host:port, specifying the Raft peers",
		&peerstr, "0:127.0.0.1:6543",
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	PreviousShmemStartupHook = shmem_startup_hook;

	if (wcfg.id >= 0)
	{
		parse_peers(wcfg.peers, peerstr);

		request_shmem();
		worker_register(&wcfg);

		shmem_startup_hook = startup_shmem;
	}
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}

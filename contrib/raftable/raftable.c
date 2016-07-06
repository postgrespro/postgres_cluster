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

#include "raft.h"
#include "util.h"

#include "raftable.h"
#include "worker.h"
#include "state.h"
#include "timeout.h"

#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(raftable_sql_get_local);
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


static bool poll_until_writable(int sock, timeout_t *timeout)
{
	struct pollfd pfd = {sock, POLLOUT, 0};
	int r = poll(&pfd, 1, timeout_remaining_ms(timeout));
	if (r != 1) return false;
	return (pfd.revents & POLLOUT) != 0;
}

static bool poll_until_readable(int sock, timeout_t *timeout)
{
	struct pollfd pfd = {sock, POLLIN, 0};
	int remain = timeout_remaining_ms(timeout);
	int r = poll(&pfd, 1, remain);
	if (r != 1) return false;
	return (pfd.revents & POLLIN) != 0;
}

static bool timed_write(int sock, void *data, size_t len, timeout_t *timeout)
{
	int sent = 0;

	while (sent < len)
	{
		int newbytes;
		if (timeout_happened(timeout))
		{
			elog(WARNING, "write timed out");
			return false;
		}

		newbytes = write(sock, (char *)data + sent, len - sent);
		if (newbytes == -1)
		{
			if (errno == EAGAIN) {
				if (poll_until_writable(sock, timeout)) {
					continue;
				}
			}
			elog(WARNING, "failed to write: error %d: %s", errno, strerror(errno));
			return false;
		}
		sent += newbytes;
	}

	return true;
}

static bool timed_read(int sock, void *data, size_t len, timeout_t *timeout)
{
	int recved = 0;

	while (recved < len)
	{
		int newbytes;
		if (timeout_happened(timeout))
		{
			elog(WARNING, "read timed out");
			return false;
		}

		newbytes = read(sock, (char *)data + recved, len - recved);
		if (newbytes <= 0)
		{
			if (errno == EAGAIN) {
				if (poll_until_readable(sock, timeout)) {
					continue;
				}
			}
			elog(WARNING, "failed to read: error %d: %s", errno, strerror(errno));
			return false;
		}
		recved += newbytes;
	}

	return true;
}

static bool connect_leader(timeout_t *timeout)
{
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;
	int rc;
	int sd;

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
		elog(WARNING, "failed to resolve address '%s:%d': %s",
			 leaderhp->host, leaderhp->port,
			 gai_strerror(rc));
		return false;
	}

	elog(WARNING, "trying [%d] %s:%d", *shared.leader, leaderhp->host, leaderhp->port);
	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int one = 1;

		sd = socket(a->ai_family, SOCK_STREAM, 0);
		if (sd == -1)
		{
			elog(WARNING, "failed to create a socket: %s", strerror(errno));
			continue;
		}
		fcntl(sd, F_SETFL, O_NONBLOCK);
		setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

		if (connect(sd, a->ai_addr, a->ai_addrlen) == -1)
		{
			if (errno == EINPROGRESS)
			{
				TIMEOUT_LOOP_START(timeout);
				{
					if (poll_until_writable(sd, timeout))
					{
						int err;
						socklen_t optlen = sizeof(err);
						getsockopt(sd, SOL_SOCKET, SO_ERROR, &err, &optlen);
						if (err == 0) goto success;
					}
				}
				TIMEOUT_LOOP_END(timeout);
				elog(WARNING, "connect timed out");
				goto failure;
			}
			else
			{
				elog(WARNING, "failed to connect to an address: %s", strerror(errno));
				close(sd);
				continue;
			}
		}

		goto success;
	}
failure:
	freeaddrinfo(addrs);
	disconnect_leader();
	elog(WARNING, "could not connect");
	return false;
success:
	freeaddrinfo(addrs);
	leadersock = sd;
	return true;
}

static void wait_ms(int ms)
{
		struct timespec ts = {0, ms * 1000000};
		nanosleep(&ts, NULL);
}

static int get_connection(timeout_t *timeout)
{
	if (leadersock < 0)
	{
		if (connect_leader(timeout)) return leadersock;
		elog(WARNING, "update: connect_leader() failed");
		wait_ms(100);
	}
	return leadersock;
}

char *raftable_get_local(const char *key, size_t *len)
{
	Assert(wcfg.id >= 0);
	return state_get(shared.state, key, len);
}

Datum
raftable_sql_get_local(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	size_t len;
	char *s;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	Assert(shared.state);

	s = raftable_get_local(shared.state, key.data, &len);
	if (s)
	{
		text *t = cstring_to_text_with_len(s, len);
		pfree(s);
		PG_RETURN_TEXT_P(t);
	}
	else
		PG_RETURN_NULL();
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	size_t len;
	char *s;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));
	int timeout_ms = PG_GETARG_INT32(1);

	s = raftable_get(key.data, &len, timeout_ms);
	if (s)
	{
		text *t = cstring_to_text_with_len(s, len);
		pfree(s);
		PG_RETURN_TEXT_P(t);
	}
	else
		PG_RETURN_NULL();
}

static RaftableMessage *raftable_try_query(RaftableMessage *msg, size_t size, size_t *rsize, timeout_t *timeout)
{
	int s;
	RaftableMessage *answer;

	s = get_connection(timeout);
	if (s < 0) return false;

	if (timeout_happened(timeout))
	{
		elog(WARNING, "query: get_connection() timed out");
		return NULL;
	}

	if (!timed_write(s, &size, sizeof(size), timeout))
	{
		elog(WARNING, "query: failed to send the query size to the leader");
		return NULL;
	}

	if (!timed_write(s, msg, size, timeout))
	{
		elog(WARNING, "query: failed to send the query to the leader");
		return NULL;
	}

	if (!timed_read(s, rsize, sizeof(size), timeout))
	{
		elog(WARNING, "query: failed to recv the answer size from the leader");
		return NULL;
	}

	if (*rsize == 0)
	{
		elog(WARNING, "query: the leader returned zero size");
		return NULL;
	}

	answer = (RaftableMessage *)palloc(*size);
	if (!timed_read(s, answer, *size, timeout))
	{
		elog(WARNING, "query: failed to recv the answer from the leader");
		pfree(answer);
		return NULL;
	}

	return answer;
}

static RaftableMessage *raftable_query(RaftableMessage *msg, size_t size, size_t *rsize, int timeout_ms)
{
	timeout_t timeout;

	if (timeout_ms < 0)
	{
		while (true)
		{
			timeout_start(&timeout, 100);

			answer = raftable_try_query(msg, size, rsize, &timeout);
			if (answer)
				return answer;
			else
				disconnect_leader();
		}
	}
	else
	{
		timeout_start(&timeout, timeout_ms);

		TIMEOUT_LOOP_START(&timeout);
		{
			answer = raftable_try_query(msg, size, rsize, &timeout);
			if (answer)
				return answer;
			else
				disconnect_leader();
		}
		TIMEOUT_LOOP_END(&timeout);
	}

	elog(WARNING, "raftable query failed after %d ms", timeout_elapsed_ms(&timeout));
	return NULL;
}

char *raftable_get(const char *key, size_t *len, int timeout_ms)
{
	RaftableMessage *msg, *answer;
	size_t size;
	size_t rsize;

	char *value = NULL;

	msg = make_single_value_message(key, NULL, 0, &size);

	Assert(wcfg.id >= 0);
	msg->expector = wcfg.id;
	msg->action = ACTION_GET;

	answer = raftable_query(msg, size, &rsize, timeout_ms);
	pfree(msg);

	if (answer)
	{
		if (answer->meaning == MEAN_OK)
		{
			RaftableField *f;
			Assert(answer->fieldnum == 1);
			f = (RaftableField *)answer->data;
			*len = f->vallen;
			if (*len)
			{
				value = palloc(*len);
				memcpy(value, f->data, *len);
			}
		}
		else
			assert(answer->meaning == MEAN_FAIL);
		pfree(answer);
	}
	return value;
}


bool raftable_set(const char *key, const char *value, size_t vallen, int timeout_ms)
{
	RaftableMessage *msg, *answer;
	size_t size;
	size_t rsize;

	bool ok = false;

	msg = make_single_value_message(key, value, vallen, &size);

	Assert(wcfg.id >= 0);
	msg->expector = wcfg.id;
	msg->action = ACTION_SET;

	answer = raftable_query(msg, size, &rsize, timeout_ms);
	pfree(msg);

	if (answer)
	{
		if (answer->meaning == MEAN_OK)
			ok = true;
		else
			assert(answer->meaning == MEAN_FAIL);
		pfree(answer);
	}
	return ok;
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

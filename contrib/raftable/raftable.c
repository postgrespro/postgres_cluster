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

static bool connect_leader(void)
{
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;
	int rc;

	if (*shared.leader == NOBODY) select_next_peer();

	HostPort *leaderhp = wcfg.peers + *shared.leader;

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
	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int one = 1;

		int sd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
		if (sd == -1)
		{
			perror("failed to create a socket");
			continue;
		}
		setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

		if (connect(sd, a->ai_addr, a->ai_addrlen) == -1)
		{
			perror("failed to connect to an address");
			close(sd);
			continue;
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

static int get_connection(void)
{
	if (leadersock < 0)
	{
		if (connect_leader()) return leadersock;

		int timeout_ms = 100;
		struct timespec timeout = {0, timeout_ms * 1000000};
		nanosleep(&timeout, NULL);
	}
	return leadersock;
}

char *raftable_get(const char *key, size_t *len)
{
	return state_get(shared.state, key, len);
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableEntry *e;
	RaftableKey key;
	size_t len;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	Assert(shared.state);

	char *s = state_get(shared.state, key.data, &len);
	if (s)
	{
		text *t = cstring_to_text_with_len(s, len);
		pfree(s);
		PG_RETURN_TEXT_P(t);
	}
	else
		PG_RETURN_NULL();
}

static void start_timer(TimestampTz *timer)
{
        *timer -= GetCurrentTimestamp();
}

static void stop_timer(TimestampTz *timer)
{
        *timer += GetCurrentTimestamp();
}

static long msec(TimestampTz timer)
{
        long sec;
        int usec;
        TimestampDifference(0, timer, &sec, &usec);
        return sec * 1000 + usec / 1000;
}

static bool try_sending_update(RaftableUpdate *ru, size_t size)
{
	int s = get_connection();

	if (s < 0) return false;

	int sent = 0, recved = 0;
	int status;

	if (write(s, &size, sizeof(size)) != sizeof(size))
	{
		disconnect_leader();
		elog(WARNING, "failed to send the update size to the leader");
		return false;
	}

	while (sent < size)
	{
		int newbytes = write(s, (char *)ru + sent, size - sent);
		if (newbytes == -1)
		{
			disconnect_leader();
			elog(WARNING, "failed to send the update to the leader");
			return false;
		}
		sent += newbytes;
	}

	recved = read(s, &status, sizeof(status));
	if (recved != sizeof(status))
	{
		disconnect_leader();
		elog(WARNING, "failed to recv the update status from the leader");
		return false;
	}

	if (status != 1)
	{
		disconnect_leader();
		elog(WARNING, "leader returned %d", status);
		return false;
	}

	return true;
}

bool raftable_set(const char *key, const char *value, size_t vallen, int timeout_ms)
{
	RaftableUpdate *ru;
	size_t size = sizeof(RaftableUpdate);
	size_t keylen = 0;
	TimestampTz now;
	int elapsed_ms;

	keylen = strlen(key) + 1;

	size += sizeof(RaftableField) - 1;
	size += keylen;
	size += vallen;
	ru = palloc(size);

	ru->expector = wcfg.id;
	ru->fieldnum = 1;

	RaftableField *f = (RaftableField *)ru->data;
	f->keylen = keylen;
	f->vallen = vallen;
	memcpy(f->data, key, keylen);
	memcpy(f->data + keylen, value, vallen);

	elapsed_ms = 0;
	now = GetCurrentTimestamp();
	while ((elapsed_ms <= timeout_ms) || (timeout_ms == -1))
	{
		TimestampTz past = now;
		if (try_sending_update(ru, size))
		{
			pfree(ru);
			return true;
		}
		now = GetCurrentTimestamp();
		elapsed_ms += msec(now - past);
	}

	pfree(ru);
	elog(WARNING, "failed to set raftable value after %d ms", timeout_ms);
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
		&wcfg.id, 0,
		0, RAFTABLE_PEERS_MAX-1,
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
	parse_peers(wcfg.peers, peerstr);

	request_shmem();
	worker_register(&wcfg);

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = startup_shmem;
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}

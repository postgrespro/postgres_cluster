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

static void try_next_peer(void)
{
	while (!wcfg.peers[*shared.leader].up)
		*shared.leader = (*shared.leader + 1) % RAFTABLE_PEERS_MAX;
}

static void disconnect_leader(void)
{
	if (leadersock >= 0)
	{
		close(leadersock);
	}
	try_next_peer();
	leadersock = -1;
}

static bool connect_leader(void)
{
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;

	HostPort *leaderhp = wcfg.peers + *shared.leader;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", leaderhp->port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;

	if (getaddrinfo(leaderhp->host, portstr, &hint, &addrs))
	{
		disconnect_leader();
		perror("failed to resolve address");
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
	while (leadersock < 0)
	{
		if (connect_leader()) break;

		int timeout_ms = 1000;
		struct timespec timeout = {0, timeout_ms * 1000000};
		nanosleep(&timeout, NULL);
	}
	return leadersock;
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableEntry *e;
	RaftableKey key;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	Assert(shared.state);

	char *s = state_get(shared.state, key.data);
	if (s)
	{
		text *t = cstring_to_text(s);
		pfree(s);
		PG_RETURN_TEXT_P(t);
	}
	else
		PG_RETURN_NULL();
}

void raftable_set(char *key, char *value)
{
	RaftableUpdate *ru;
	size_t size = sizeof(RaftableUpdate);
	int keylen, vallen = 0;

	keylen = strlen(key) + 1;
	if (value) vallen = strlen(value) + 1;

	size += sizeof(RaftableField) - 1;
	size += keylen;
	size += vallen;
	ru = palloc(size);

	RaftableField *f = (RaftableField *)ru->data;
	f->keylen = keylen;
	f->vallen = vallen;
	memcpy(f->data, key, keylen);
	memcpy(f->data + keylen, key, vallen);

	bool ok = false;
	while (!ok)
	{
		fprintf(stderr, "trying to send an update to the leader\n");
		int s = get_connection();
		int sent = 0;
		ok = true;

		if (write(s, &size, sizeof(size)) != sizeof(size))
		{
			disconnect_leader();
			fprintf(stderr, "failed to send the update size to the leader\n");
			ok = false;
			continue;
		}

		while (ok && (sent < size))
		{
			int newbytes = write(s, (char *)ru + sent, size - sent);
			if (newbytes == -1)
			{
				disconnect_leader();
				fprintf(stderr, "failed to send the update to the leader\n");
				ok = false;
			}
			sent += newbytes;
		}
	}

	pfree(ru);
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	char *key = text_to_cstring(PG_GETARG_TEXT_P(0));
	if (PG_ARGISNULL(1))
		raftable_set(key, NULL);
	else
	{
		char *value = text_to_cstring(PG_GETARG_TEXT_P(1));
		raftable_set(key, value);
		pfree(value);
	}
	pfree(key);

	PG_RETURN_VOID();
}

Datum
raftable_sql_list(PG_FUNCTION_ARGS)
{
	char *key, *value;
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

	if (state_next(shared.state, funcctx->user_fctx, &key, &value))
	{
		HeapTuple tuple;
		Datum  vals[2];
		bool isnull[2];

		vals[0] = CStringGetTextDatum(key);
		vals[1] = CStringGetTextDatum(value);
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

	worker_register(&wcfg);

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = startup_shmem;
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}

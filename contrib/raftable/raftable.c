/*
 * raftable.c
 *
 * A key-value table replicated over Raft.
 *
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "storage/lwlock.h"
#include "storage/ipc.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "postmaster/bgworker.h"
#include "miscadmin.h"

#include "raftable.h"
#include "raft.h"
#include "util.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define RAFTABLE_PEERS_MAX (64)

#define RAFTABLE_KEY_LEN (64)
#define RAFTABLE_BLOCK_LEN (256)
#define RAFTABLE_BLOCKS (4096)
#define RAFTABLE_BLOCK_MEM (RAFTABLE_BLOCK_LEN * (RAFTABLE_BLOCKS - 1) + sizeof(RaftableBlockMem))
#define RAFTABLE_HASH_SIZE (127)
#define RAFTABLE_SHMEM_SIZE ((1024 * 1024) + RAFTABLE_BLOCK_MEM)

#define LISTEN_QUEUE_SIZE 10

typedef struct RaftableBlock
{
	struct RaftableBlock *next;
	char data[RAFTABLE_BLOCK_LEN - sizeof(void*)];
} RaftableBlock;

typedef struct RaftableKey
{
	char data[RAFTABLE_KEY_LEN];
} RaftableKey;

typedef struct RaftableEntry
{
	RaftableKey key;
	int len;
	RaftableBlock *value;
} RaftableEntry;

typedef struct RaftableBlockMem
{
	RaftableBlock *free_blocks;
	RaftableBlock blocks[1];
} RaftableBlockMem;

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(raftable_sql_get);
PG_FUNCTION_INFO_V1(raftable_sql_set);
PG_FUNCTION_INFO_V1(raftable_sql_list);

static raft_config_t raft_config;
static raft_t raft;

typedef struct client_t {
} client_t;

#define MAX_CLIENTS 1024
struct {
	char *host;
	int port;

	int listener;
	int raftsock;

	fd_set all;
	int maxfd;
	int clients[MAX_CLIENTS];
	int clientnum;
} server;

static int raftable_id;
static char *raftable_peers;

static HTAB *hashtable;
static LWLockId hashlock;

static RaftableBlockMem *blockmem;
static LWLockId blocklock;

static shmem_startup_hook_type PreviousShmemStartupHook;

static void raftable_worker_main(Datum);

static BackgroundWorker raftable_worker = {
	"raftable worker",
	0, // flags
	BgWorkerStart_PostmasterStart,
	1, // restart time
	raftable_worker_main // main
};

static RaftableBlock *block_alloc(void)
{
	RaftableBlock *result;

	LWLockAcquire(blocklock, LW_EXCLUSIVE);

	result = blockmem->free_blocks;
	if (result == NULL)
		elog(ERROR, "raftable memory limit hit");


	blockmem->free_blocks = blockmem->free_blocks->next;
	result->next = NULL;
	LWLockRelease(blocklock);
	return result;
}

static void block_free(RaftableBlock *block)
{
	RaftableBlock *new_free_head = block;
	Assert(block != NULL);
	while (block->next != NULL)
	{
		block = block->next;
	}
	LWLockAcquire(blocklock, LW_EXCLUSIVE);
	block->next = blockmem->free_blocks;
	blockmem->free_blocks = new_free_head;
	LWLockRelease(blocklock);
}


static text *entry_to_text(RaftableEntry *e)
{
	char *cursor, *buf;
	RaftableBlock *block;
	text *t;
	int len;

	buf = palloc(e->len + 1);
	cursor = buf;

	block = e->value;
	len = e->len;
	while (block != NULL)
	{
		int tocopy = len;
		if (tocopy > sizeof(block->data))
			tocopy = sizeof(block->data);

		memcpy(cursor, block->data, tocopy);
		cursor += tocopy;
		len -= tocopy;

		Assert(cursor - buf <= e->len);
		block = block->next;
	}
	Assert(cursor - buf == e->len);
	*cursor = '\0';
	t = cstring_to_text_with_len(buf, e->len);
	pfree(buf);
	return t;
}

static void text_to_entry(RaftableEntry *e, text *t)
{
	char *buf, *cursor;
	int len;
	RaftableBlock *block;

	buf = text_to_cstring(t);
	cursor = buf;
	len = strlen(buf);
	e->len = len;

	if (e->len > 0)
	{
		if (e->value == NULL)
			e->value = block_alloc();
		Assert(e->value != NULL);
	}
	else
	{
		if (e->value != NULL)
		{
			block_free(e->value);
			e->value = NULL;
		}
	}

	block = e->value;
	while (len > 0)
	{
		int tocopy = len;
		if (tocopy > sizeof(block->data))
		{
			tocopy = sizeof(block->data);
			if (block->next == NULL)
				block->next = block_alloc();
		}
		else
		{
			if (block->next != NULL)
			{
				block_free(block->next);
				block->next = NULL;
			}
		}

		memcpy(block->data, cursor, tocopy);
		cursor += tocopy;
		len -= tocopy;

		block = block->next;
	}

	pfree(buf);
	Assert(block == NULL);
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableEntry *entry;
	RaftableKey key;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	LWLockAcquire(hashlock, LW_SHARED);
	entry = hash_search(hashtable, &key, HASH_FIND, NULL);

	if (entry)
	{
		text *t = entry_to_text(entry);
		LWLockRelease(hashlock);
		PG_RETURN_TEXT_P(t);
	}
	else
	{
		LWLockRelease(hashlock);
		PG_RETURN_NULL();
	}
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));

	LWLockAcquire(hashlock, LW_EXCLUSIVE);
	if (PG_ARGISNULL(1))
	{
		RaftableEntry *entry = hash_search(hashtable, key.data, HASH_FIND, NULL);
		if ((entry != NULL) && (entry->len > 0))
			block_free(entry->value);
		hash_search(hashtable, key.data, HASH_REMOVE, NULL);
	}
	else
	{
		bool found;
		RaftableEntry *entry = hash_search(hashtable, key.data, HASH_ENTER, &found);
		if (!found)
		{
			entry->key = key;
			entry->value = NULL;
			entry->len = 0;
		}
		text_to_entry(entry, PG_GETARG_TEXT_P(1));
	}
	LWLockRelease(hashlock);

	PG_RETURN_VOID();
}

Datum
raftable_sql_list(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	MemoryContext oldcontext;
	HASH_SEQ_STATUS *scan;
	RaftableEntry *entry;

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

		scan = (HASH_SEQ_STATUS *)palloc(sizeof(HASH_SEQ_STATUS));
		LWLockAcquire(hashlock, LW_SHARED);
		hash_seq_init(scan, hashtable);

		MemoryContextSwitchTo(oldcontext);

		funcctx->user_fctx = scan;
	}

	funcctx = SRF_PERCALL_SETUP();
	scan = funcctx->user_fctx;

	if ((entry = (RaftableEntry *)hash_seq_search(scan)))
	{
		HeapTuple tuple;
		Datum  vals[2];
		bool isnull[2];

		vals[0] = CStringGetTextDatum(entry->key.data);
		vals[1] = PointerGetDatum(entry_to_text(entry));
		isnull[0] = isnull[1] = false;

		tuple = heap_form_tuple(funcctx->tuple_desc, vals, isnull);
		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		LWLockRelease(hashlock);
		SRF_RETURN_DONE(funcctx);
	}

}

static void startup_shmem(void)
{
	if (PreviousShmemStartupHook)
	{
		PreviousShmemStartupHook();
	}

	{
		HASHCTL info;
		hashlock = LWLockAssign();

		info.keysize = sizeof(RaftableKey);
		info.entrysize = sizeof(RaftableEntry);

		hashtable = ShmemInitHash(
			"raftable_hashtable",
			RAFTABLE_HASH_SIZE, RAFTABLE_HASH_SIZE,
			&info, HASH_ELEM
		);
	}

	{
		bool found;
		int i;

		blocklock = LWLockAssign();

		blockmem = ShmemInitStruct(
			"raftable_blockmem",
			RAFTABLE_BLOCK_MEM,
			&found
		);

		for (i = 0; i < RAFTABLE_BLOCKS - 1; i++)
		{
			blockmem->blocks[i].next = blockmem->blocks + i + 1;
		}
		blockmem->blocks[RAFTABLE_BLOCKS - 1].next = NULL;
		blockmem->free_blocks = blockmem->blocks;
	}
}

//int strsplit(char **words, int maxwnum, char *src, char delim)
//{
//	char *d;
//	int wnum = 0;
//	int len;
//
//	if (maxwnum < 1) return 0;
//
//	while ((d = strchr(src, delim)))
//	{
//		len = d - src;
//		words[wnum] = palloc(len + 1);
//		memcpy(words[wnum], src, len);
//		words[wnum][len] = '\0';
//
//		wnum++;
//		if (wnum >= maxwnum) return wnum;
//		src = d + 1;
//	}
//
//	len = strlen(src);
//	words[wnum] = palloc(len + 1);
//	memcpy(words[wnum], src, len);
//	words[wnum][len] = '\0';
//
//	return wnum;
//}

static void add_peers(int myid, char *peers)
{
	char *state, *substate;
	char *peer, *s;
	char *host;
	int id, port;
	int i;

	fprintf(stderr, "parsing '%s'\n", peers);
	peer = strtok_r(peers, ",", &state);
	while (peer)
	{
		fprintf(stderr, "peer = '%s'\n", peer);

		s = strtok_r(peer, ":", &substate);
		if (!s) break;
		id = atoi(s);
		fprintf(stderr, "id = %d ('%s')\n", id, s);

		host = strtok_r(NULL, ":", &substate);
		if (!host) break;
		fprintf(stderr, "host = '%s'\n", host);

		s = strtok_r(NULL, ":", &substate);
		if (!s) break;
		port = atoi(s);
		fprintf(stderr, "port = %d ('%s')\n", port, s);

		if (id == myid)
		{
			raft_peer_up(raft, id, host, port, true);
			server.host = host;
			server.port = port;
		}
		else
			raft_peer_up(raft, id, host, port, false);

		peer = strtok_r(NULL, ",", &state);
	}
}

/* Returns the created socket, or -1 if failed. */
static int create_listening_socket(const char *host, int port) {
	int optval;
	struct sockaddr_in addr;
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		fprintf(stderr, "cannot create the listening socket: %s\n", strerror(errno));
		return -1;
	}

	optval = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char const*)&optval, sizeof(optval));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char const*)&optval, sizeof(optval));

	addr.sin_family = AF_INET;
	if (inet_aton(host, &addr.sin_addr) == 0) {
		fprintf(stderr, "cannot convert the host string '%s' to a valid address\n", host);
		return -1;
	}
	addr.sin_port = htons(port);
	fprintf(stderr, "binding tcp %s:%d\n", host, port);
	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		fprintf(stderr, "cannot bind the listening socket: %s\n", strerror(errno));
		return -1;
	}

	if (listen(s, LISTEN_QUEUE_SIZE) == -1) {
		fprintf(stderr, "failed to listen the socket: %s\n", strerror(errno));
		return -1;
	}    

	return s;
}

static bool server_add_socket(int sock)
{
	FD_SET(sock, &server.all);
	if (sock > server.maxfd) {
		server.maxfd = sock;
	}
	return true;
}

static bool server_add_client(int sock)
{
	int i = 0;
	int cnum = server.clientnum;

	if (cnum >= MAX_CLIENTS)
	{
		fprintf(stderr, "client limit hit\n");
		return false;
	}

	while (cnum > 0)
	{
		if (server.clients[i] >= 0) cnum--;
		i++;
	}

	server.clients[i] = sock;
	server.clientnum++;
	return server_add_socket(sock);
}

static bool server_remove_socket(int sock)
{
	FD_CLR(sock, &server.all);
	return true;
}

static bool server_remove_client(int sock)
{
	int i = 0;
	int cnum = server.clientnum;

	if (cnum <= 0) return false;

	while (cnum > 0)
	{
		if (server.clients[i] >= 0) cnum--;
		if (server.clients[i] == sock) break;
		i++;
	}

	if (server.clients[i] != sock) return false;

	server.clients[i] = -1;
	server.clientnum--;
	close(sock);
	return server_remove_socket(sock);
}

static bool server_start()
{
	server.listener = -1;
	server.raftsock = -1;
	FD_ZERO(&server.all);
	server.maxfd = 0;
	server.clientnum = 0;

	server.listener = create_listening_socket(server.host, server.port);
	if (server.listener == -1) {
		return false;
	}

	return server_add_socket(server.listener);
}

static bool server_accept()
{
	int fd;

	fprintf(stderr, "a new connection is queued\n");

	fd = accept(server.listener, NULL, NULL);
	if (fd == -1) {
		fprintf(stderr, "failed to accept a connection: %s\n", strerror(errno));
		return false;
	}
	fprintf(stderr, "a new connection fd=%d accepted\n", fd);
	
	if (!raft_is_leader(raft)) {
		fprintf(stderr, "not a leader, disconnecting the accepted connection fd=%d\n", fd);
		close(fd);
		return false;
	}

	return server_add_client(fd);
}

int server_handle(int sock)
{
}

bool server_tick(int timeout_ms)
{
	int i;
	int numready;
	bool raft_ready = false;

	fd_set readfds = server.all;
	struct timeval timeout = ms2tv(timeout_ms);
	numready = select(server.maxfd + 1, &readfds, NULL, NULL, &timeout);
	if (numready == -1) {
		fprintf(stderr, "failed to select: %s\n", strerror(errno));
		return false;
	}

	if (FD_ISSET(server.listener, &readfds)) {
		numready--;
		server_accept();
	}

	if (FD_ISSET(server.raftsock, &readfds)) {
		numready--;
		raft_ready = true;
	}

	i = 0;
	while (numready > 0)
	{
		int sock = server.clients[i];
		if ((sock >= 0) && (FD_ISSET(sock, &readfds)))
		{
			server_handle(sock);
			numready--;
		}
	}

	return raft_ready;
}

static int stop = 0;
static void die(int sig)
{
    stop = 1;
}

static void raftable_worker_main(Datum arg)
{
	if (!server_start()) elog(ERROR, "couldn't start raftable server");

    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGTERM, die);
    sigset_t sset;
    sigfillset(&sset);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);

	server.raftsock = raft_create_udp_socket(raft);
	server_add_socket(server.raftsock);
	server_add_socket(server.listener);
	if (server.raftsock == -1) elog(ERROR, "couldn't start raft");

	mstimer_t t;
	mstimer_reset(&t);
	while (!stop)
	{
		raft_msg_t m = NULL;

		int ms = mstimer_reset(&t);
		raft_tick(raft, ms);

		if (server_tick(raft_config.heartbeat_ms))
		{
			m = raft_recv_message(raft);
			Assert(m != NULL);
			raft_handle_message(raft, m);
		}
	}
}

void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
		elog(ERROR, "please add 'raftable' to shared_preload_libraries list");

	raft_config.peernum_max = RAFTABLE_PEERS_MAX;

	DefineCustomIntVariable("raftable.id",
		"Raft peer id of current instance", NULL,
		&raftable_id, 0,
		0, RAFTABLE_PEERS_MAX-1,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.heartbeat_ms",
		"Raft heartbeat timeout in ms", NULL,
		&raft_config.heartbeat_ms, 20,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.election_ms_min",
		"Raft min election timeout in ms", NULL,
		&raft_config.election_ms_min, 150,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.election_ms_max",
		"Raft max election timeout in ms", NULL,
		&raft_config.election_ms_max, 300,
		0, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.log_len",
		"Raft log length", NULL,
		&raft_config.log_len, 512,
		8, INT_MAX,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.chunk_len",
		"Raft chunk length", NULL,
		&raft_config.chunk_len, 400,
		1, 400,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomIntVariable("raftable.msg_len_max",
		"Raft chunk length", NULL,
		&raft_config.msg_len_max, 500,
		1, 500,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	DefineCustomStringVariable("raftable.peers",
		"Raft peer list",
		"A comma separated list of id:host:port, specifying the Raft peers",
		&raftable_peers, "0:127.0.0.1:6543",
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);

	raft = raft_init(&raft_config);
	if (raft == NULL)
	{
		elog(ERROR, "couldn't configure raft");
	}

	raftable_peers = strdup(raftable_peers);
	add_peers(raftable_id, raftable_peers);

	RequestAddinShmemSpace(RAFTABLE_SHMEM_SIZE);
	RequestAddinLWLocks(2);

	RegisterBackgroundWorker(&raftable_worker);

	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = startup_shmem;
}

void
_PG_fini(void)
{
	shmem_startup_hook = PreviousShmemStartupHook;
}

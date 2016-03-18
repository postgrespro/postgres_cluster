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
#include <time.h>

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

#define MAX_CLIENTS 1024
#define BUFLEN 1024

typedef struct client_t {
	bool good;
	int sock;
	char buf[BUFLEN];
	size_t bufrecved;
	char *msg;
	size_t msgrecved;
	size_t msglen;
	int expected_version;
} client_t;

struct {
	char *host;
	int port;

	int listener;
	int raftsock;

	fd_set all;
	int maxfd;
	int clientnum;
	client_t clients[MAX_CLIENTS];
} server;

typedef struct host_port_t {
	char *host;
	int port;
} host_port_t;

static int raftable_id;
static char *raftable_peers;
static host_port_t raftable_peer_addr[RAFTABLE_PEERS_MAX];
static int leadersock = -1;

static void disconnect_leader()
{
	if (leadersock >= 0)
	{
		close(leadersock);
	}
	leadersock = -1;
}

static bool connect_leader()
{
	// use an IP socket
	struct addrinfo *addrs = NULL;
	struct addrinfo hint;
	char portstr[6];
	struct addrinfo *a;

	int leader = raft_get_leader(raft);
	if (leader == NOBODY)
	{
		fprintf(stderr, "no leader to connect to\n");
		return false;
	}

	host_port_t *leaderhp = raftable_peer_addr + leader;

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

		// success
		freeaddrinfo(addrs);
		leadersock = sd;
		return true;
	}
	freeaddrinfo(addrs);
	disconnect_leader();
	fprintf(stderr, "could not connect\n");
	return false;
}

static int get_connection()
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

static char *entry_to_string(RaftableEntry *e)
{
	char *cursor, *s;
	RaftableBlock *block;
	text *t;
	int len;

	s = palloc(e->len + 1);
	cursor = s;

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

		Assert(cursor - s <= e->len);
		block = block->next;
	}
	Assert(cursor - s == e->len);
	*cursor = '\0';
	return s;
}

static text *entry_to_text(RaftableEntry *e)
{
	text *t;
	char *s = entry_to_string(e);
	t = cstring_to_text_with_len(s, e->len);
	pfree(s);
	return t;
}

static void string_to_entry(RaftableEntry *e, char *value)
{
	int len;
	RaftableBlock *block;

	len = strlen(value);
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

		memcpy(block->data, value, tocopy);
		value += tocopy;
		len -= tocopy;

		block = block->next;
	}

	Assert(block == NULL);
}

static void text_to_entry(RaftableEntry *e, text *t)
{
	char *s = text_to_cstring(t);
	string_to_entry(e, s);
	pfree(s);
}

static void local_state_clear()
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *entry;

	hash_seq_init(&scan, hashtable);
	while ((entry = (RaftableEntry *)hash_seq_search(&scan))) {
		if (entry->len > 0)
			block_free(entry->value);
		hash_search(hashtable, entry->key.data, HASH_REMOVE, NULL);
	}
	hash_seq_term(&scan);
}

typedef struct raftable_field_t {
	int keylen; // with NULL at the end
	int vallen; // with NULL at the end
	bool isnull;
	char data[1];
} raftable_field_t;

typedef struct raftable_update_t {
	int version;
	int fieldnum;
	char data[1];
} raftable_update_t;

static void local_state_set(char *key, char *value)
{
	if (value == NULL)
	{
		RaftableEntry *entry = hash_search(hashtable, key, HASH_FIND, NULL);
		if ((entry != NULL) && (entry->len > 0))
			block_free(entry->value);
		hash_search(hashtable, key, HASH_REMOVE, NULL);
	}
	else
	{
		bool found;
		RaftableEntry *entry = hash_search(hashtable, key, HASH_ENTER, &found);
		if (!found)
		{
			strncpy(entry->key.data, key, RAFTABLE_KEY_LEN);
			entry->key.data[RAFTABLE_KEY_LEN - 1] = '\0';
			entry->value = NULL;
			entry->len = 0;
		}
		string_to_entry(entry, value);
	}
}

static void local_state_update(raftable_update_t *update)
{
	raftable_field_t *f;
	int i;
	char *cursor = update->data;
	for (i = 0; i < update->fieldnum; i++) {
		f = (raftable_field_t *)cursor;
		cursor = f->data;
		char *key = cursor; cursor += f->keylen;
		char *value = cursor; cursor += f->vallen;
		local_state_set(key, value);
	}
}

static bool notify(int version)
{
	int i = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		client_t *c = server.clients + i;
		if (c->sock < 0) continue;
		if (!c->good) continue;
		if (!c->expected_version) continue;
		if (version < c->expected_version) continue;

		if (send(c->sock, &c->expected_version, sizeof(c->expected_version), 0) != sizeof(c->expected_version))
		{
			fprintf(stderr, "failed to notify client\n");
			c->good = false;
		}
		c->expected_version = 0;
	}
}

static void applier(void *state, raft_update_t update, bool snapshot)
{
	raftable_update_t *ru = (raftable_update_t *)update.data;
	Assert(state == hashtable);

	LWLockAcquire(hashlock, LW_EXCLUSIVE);

	if (snapshot) local_state_clear();
	local_state_update(ru);
	notify(ru->version);

	LWLockRelease(hashlock);
}

static size_t estimate_snapshot_size()
{
	HASH_SEQ_STATUS scan;
	RaftableEntry *entry;
	size_t size = sizeof(raftable_update_t);

	hash_seq_init(&scan, hashtable);
	while ((entry = (RaftableEntry *)hash_seq_search(&scan))) {
		size += sizeof(raftable_field_t) - 1;
		size += strlen(entry->key.data) + 1;
		size += entry->len + 1;
	}
	hash_seq_term(&scan);

	return size;
}

static raft_update_t snapshooter(void *state) {
	HASH_SEQ_STATUS scan;
	RaftableEntry *entry;
	raft_update_t shot;
	raftable_update_t *update;
	char *cursor;

	Assert(state == hashtable);

	shot.len = estimate_snapshot_size();
	shot.data = malloc(shot.len);
	if (!shot.data) {
		fprintf(stderr, "failed to take a snapshot\n");
	}

	update = (raftable_update_t *)shot.data;
	cursor = update->data;

	hash_seq_init(&scan, hashtable);
	while ((entry = (RaftableEntry *)hash_seq_search(&scan))) {
		raftable_field_t *f = (raftable_field_t *)cursor;
		cursor = f->data;

		f->keylen = strlen(entry->key.data) + 1;
		memcpy(cursor, entry->key.data, f->keylen - 1);
		cursor[f->keylen - 1] = '\0';
		cursor += f->keylen;

		if (entry->len)
		{
			f->isnull = false;
			f->vallen = entry->len + 1;

			char *s = entry_to_string(entry);
			memcpy(cursor, s, f->vallen);
			pfree(s);

			cursor += f->vallen;
		}
		else
			f->isnull = true;
	}
	hash_seq_term(&scan);

	return shot;
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

void raftable_set(char *key, char *value)
{
	raftable_update_t *ru;
	size_t size = sizeof(raftable_update_t);
	int keylen, vallen = 0;

	keylen = strlen(key) + 1;
	if (value) vallen = strlen(value) + 1;

	size += sizeof(raftable_field_t) - 1;
	size += keylen;
	size += vallen;
	ru = palloc(size);

	raftable_field_t *f = ru->data;
	f->keylen = keylen;
	f->vallen = vallen;
	memcpy(f->data, key, keylen);
	memcpy(f->data + keylen, key, vallen);

	while (true)
	{
		fprintf(stderr, "trying to send update to the leader\n");
		int s = get_connection();
		int sent = 0;
		while (sent < size)
		{
			int newbytes = write(s, (char *)ru + sent, size - sent);
			if (newbytes == -1)
			{
				disconnect_leader();
				fprintf(stderr, "failed to send update to the leader\n");
			}
			sent += newbytes;
		}

		// FIXME: add blocking until the updated state version appears
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

static bool add_socket(int sock)
{
	FD_SET(sock, &server.all);
	if (sock > server.maxfd) {
		server.maxfd = sock;
	}
	return true;
}

static bool add_client(int sock)
{
	int i;
	client_t *c = server.clients;

	if (server.clientnum >= MAX_CLIENTS)
	{
		fprintf(stderr, "client limit hit\n");
		return false;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		client_t *c = server.clients + i;
		if (c->sock < 0) continue;

		c->sock = sock;
		c->good = true;
		c->msg = NULL;
		c->bufrecved = 0;
		c->expected_version = 0;
		server.clientnum++;
		return add_socket(sock);
	}

	Assert(false); // should not happen
	return false;
}

static bool remove_socket(int sock)
{
	FD_CLR(sock, &server.all);
	return true;
}

static bool remove_client(client_t *c)
{
	int i = 0;
	int sock = c->sock;
	Assert(sock >= 0);
	c->sock = -1;
	pfree(c->msg);

	server.clientnum--;
	close(sock);
	return remove_socket(sock);
}

static bool start_server()
{
	int i;

	server.listener = -1;
	server.raftsock = -1;
	FD_ZERO(&server.all);
	server.maxfd = 0;
	server.clientnum = 0;

	server.listener = create_listening_socket(server.host, server.port);
	if (server.listener == -1)
	{
		return false;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		server.clients[i].sock = -1;
	}

	return add_socket(server.listener);
}

static bool accept_client()
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

	return add_client(fd);
}

bool pull_from_socket(client_t *c)
{
	if (!c->good) return false;
	Assert(c->sock >= 0);
	void *dst = c->buf + c->bufrecved;
	size_t avail = BUFLEN - c->bufrecved;
	if (!avail) return false;

	size_t recved = recv(c->sock, dst, avail, MSG_DONTWAIT);
	if (recv <= 0)
	{
		c->good = false;
		return false;
	}
	c->bufrecved += recved;

	return true;
}

static void shift_buffer(client_t *c, size_t bytes)
{
	Assert(c->bufrecved >= bytes);
	memmove(c->buf, c->buf + bytes, c->bufrecved - bytes);
	c->bufrecved -= bytes;
}

static void extract_nomore(client_t *c, void *dst, size_t bytes)
{
	if (c->bufrecved < bytes) bytes = c->bufrecved;
	
	memcpy(dst, c->buf, bytes);
	shift_buffer(c, bytes);
}

static bool extract_exactly(client_t *c, void *dst, size_t bytes)
{
	if (c->bufrecved < bytes) return false;

	memcpy(dst, c->buf, bytes);
	shift_buffer(c, bytes);
	return true;
}

static bool get_new_message(client_t *c)
{
	Assert((!c->msg) || (c->msgrecved < c->msglen));
	if (!c->msg) // need to allocate the memory for the message
	{
		if (!extract_exactly(c, &c->msglen, sizeof(c->msglen)))
			return false; // but the size is still unknown

		c->msg = palloc(c->msglen);
		c->msgrecved = 0;

		if (c->msgrecved < c->msglen)
		{
		}
	}
}

void attend(client_t *c)
{
	if (!c->good) return;
	if (!pull_from_socket(c))
		return;
}

void drop_bads()
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		client_t *c = server.clients + i;
		if (c->sock < 0) continue;
		if (!c->good) remove_client(c);
	}
}

bool tick(int timeout_ms)
{
	int i;
	int numready;
	bool raft_ready = false;

	fd_set readfds = server.all;
	struct timeval timeout = ms2tv(timeout_ms);
	numready = select(server.maxfd + 1, &readfds, NULL, NULL, &timeout);
	if (numready == -1)
	{
		fprintf(stderr, "failed to select: %s\n", strerror(errno));
		return false;
	}

	if (FD_ISSET(server.listener, &readfds))
	{
		numready--;
		accept_client();
	}

	if (FD_ISSET(server.raftsock, &readfds))
	{
		numready--;
		raft_ready = true;
	}

	client_t *c = server.clients;
	while (numready > 0)
	{
		Assert(c - server.clients < MAX_CLIENTS);
		if ((c->sock >= 0) && (FD_ISSET(c->sock, &readfds)))
		{
			attend(c);
			numready--;
		}
		c++;
	}

	drop_bads();

	return raft_ready;
}

static int stop = 0;
static void die(int sig)
{
    stop = 1;
}

static void raftable_worker_main(Datum arg)
{
	if (!start_server()) elog(ERROR, "couldn't start raftable server");

    signal(SIGINT, die);
    signal(SIGQUIT, die);
    signal(SIGTERM, die);
    sigset_t sset;
    sigfillset(&sset);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);

	server.raftsock = raft_create_udp_socket(raft);
	add_socket(server.raftsock);
	add_socket(server.listener);
	if (server.raftsock == -1) elog(ERROR, "couldn't start raft");

	mstimer_t t;
	mstimer_reset(&t);
	while (!stop)
	{
		raft_msg_t m = NULL;

		int ms = mstimer_reset(&t);
		raft_tick(raft, ms);

		if (tick(raft_config.heartbeat_ms))
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

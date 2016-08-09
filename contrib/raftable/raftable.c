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
#include "postmaster/bgworker.h"
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

PG_FUNCTION_INFO_V1(raftable_sql_peer);
PG_FUNCTION_INFO_V1(raftable_sql_start);
PG_FUNCTION_INFO_V1(raftable_sql_stop);

PG_FUNCTION_INFO_V1(raftable_sql_get);
PG_FUNCTION_INFO_V1(raftable_sql_set);

static shmem_startup_hook_type PreviousShmemStartupHook;

static int leader = -1;
static int leadersock = -1;
static WorkerConfig wcfg;
static volatile WorkerConfig *sharedcfg = &wcfg;

static void select_next_server(void)
{
	int i;
	int orig_leader = leader;
	SpinLockAcquire(&sharedcfg->lock);
	for (i = 0; i < MAX_SERVERS; i++)
	{
		int idx = (orig_leader + i + 1) % MAX_SERVERS;
		volatile HostPort *hp = sharedcfg->peers + idx;
		if (hp->up)
		{
			leader = idx;
			SpinLockRelease(&sharedcfg->lock);
			return;
		}
	}
	SpinLockRelease(&sharedcfg->lock);
	shout("all raftable servers are down\n");
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
		if (timeout_happened(timeout)) return false;

		newbytes = write(sock, (char *)data + sent, len - sent);
		if (newbytes > 0)
		{
			sent += newbytes;
		}
		else if (newbytes == 0)
		{
			return false;
		}
		else
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				if (!poll_until_writable(sock, timeout)) return false;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

static bool timed_read(int sock, void *data, size_t len, timeout_t *timeout)
{
	int recved = 0;

	while (recved < len)
	{
		int newbytes;
		if (timeout_happened(timeout)) return false;

		newbytes = read(sock, (char *)data + recved, len - recved);
		if (newbytes > 0)
		{
			recved += newbytes;
		}
		else if (newbytes == 0)
		{
			return false;
		}
		else
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				if (!poll_until_readable(sock, timeout)) return false;
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

static void wait_ms(int ms) {
	struct timespec ts = {ms / 1000, (ms % 1000) * 1000000};
	struct timespec rem;
	while (nanosleep(&ts, &rem) == -1)
	{
		if (errno != EINTR) break;
		ts = rem;
	}
}


static void disconnect_leader(void)
{
	if (leadersock >= 0) close(leadersock);
	wait_ms(100);
	select_next_server();
	leadersock = -1;
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

	if (leader == -1) select_next_server();

	leaderhp = (HostPort *)sharedcfg->peers + leader;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_family = AF_INET;
	snprintf(portstr, 6, "%d", leaderhp->port);
	hint.ai_protocol = getprotobyname("tcp")->p_proto;

	if ((rc = getaddrinfo(leaderhp->host, portstr, &hint, &addrs)))
	{
		disconnect_leader();
		elog(
			WARNING,
			"raftable client: failed to resolve address '%s:%d': %s",
			leaderhp->host, leaderhp->port,
			gai_strerror(rc)
		);
		return false;
	}

	elog(WARNING, "raftable client: trying [%d] %s:%d", leader, leaderhp->host, leaderhp->port);
	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int one = 1;

		sd = socket(a->ai_family, SOCK_STREAM, 0);
		if (sd == -1)
		{
			elog(WARNING, "raftable client: failed to create a socket: %s", strerror(errno));
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
				elog(WARNING, "raftable client: connect timed out");
				goto failure;
			}
			else
			{
				elog(WARNING, "raftable client: failed to connect to an address: %s", strerror(errno));
				close(sd);
				continue;
			}
		}

		goto success;
	}
failure:
	freeaddrinfo(addrs);
	disconnect_leader();
	elog(WARNING, "raftable client: could not connect");
	return false;
success:
	freeaddrinfo(addrs);
	leadersock = sd;
	return true;
}

static int get_connection(timeout_t *timeout)
{
	if (leadersock < 0)
	{
		if (connect_leader(timeout)) return leadersock;
		elog(WARNING, "raftable client: connect_leader() failed");
	}
	return leadersock;
}

Datum
raftable_sql_peer(PG_FUNCTION_ARGS)
{
	int id;
	char *host;
	int port;

	id = PG_GETARG_INT32(0);
	host = text_to_cstring(PG_GETARG_TEXT_P(1));
	port = PG_GETARG_INT32(2);

	raftable_peer(id, host, port);
	pfree(host);

	PG_RETURN_VOID();
}

Datum
raftable_sql_start(PG_FUNCTION_ARGS)
{
	int id = PG_GETARG_INT32(0);

	pid_t pid = raftable_start(id);

	PG_RETURN_INT32(pid);
}

Datum
raftable_sql_stop(PG_FUNCTION_ARGS)
{
	raftable_stop();
	PG_RETURN_VOID();
}

Datum
raftable_sql_get(PG_FUNCTION_ARGS)
{
	RaftableKey key;
	size_t len;
	char *s;
	int timeout_ms;
	text_to_cstring_buffer(PG_GETARG_TEXT_P(0), key.data, sizeof(key.data));
	timeout_ms = PG_GETARG_INT32(1);

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

	answer = (RaftableMessage *)palloc(*rsize);
	if (!timed_read(s, answer, *rsize, timeout))
	{
		elog(WARNING, "query: failed to recv the answer from the leader");
		pfree(answer);
		return NULL;
	}

	return answer;
}

static RaftableMessage *raftable_query(RaftableMessage *msg, size_t size, size_t *rsize, int timeout_ms)
{
	RaftableMessage *answer;
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

	msg->meaning = MEAN_GET;

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
				memcpy(value, f->data + f->keylen, *len);
			}
		}
		else
			Assert(answer->meaning == MEAN_FAIL);
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

	msg->meaning = MEAN_SET;

	answer = raftable_query(msg, size, &rsize, timeout_ms);
	pfree(msg);

	if (answer)
	{
		if (answer->meaning == MEAN_OK)
			ok = true;
		else
			Assert(answer->meaning == MEAN_FAIL);
		pfree(answer);
	}
	return ok;
}

Datum
raftable_sql_set(PG_FUNCTION_ARGS)
{
	bool ok;
	char *key = text_to_cstring(PG_GETARG_TEXT_P(0));
	int timeout_ms = PG_GETARG_INT32(2);
	if (PG_ARGISNULL(1))
		ok = raftable_set(key, NULL, 0, timeout_ms);
	else
	{
		char *value = text_to_cstring(PG_GETARG_TEXT_P(1));
		ok = raftable_set(key, value, strlen(value), timeout_ms);
		pfree(value);
	}
	pfree(key);

	PG_RETURN_BOOL(ok);
}

static void
raftableShmemStartup(void)
{
	bool found;
	if (PreviousShmemStartupHook) PreviousShmemStartupHook();
	sharedcfg = (WorkerConfig *)ShmemInitStruct("raftable_config", sizeof(WorkerConfig), &found);
	if (!found) {
		*sharedcfg = wcfg;
	}
}

void
_PG_init(void)
{
	int i;
	SpinLockInit(&sharedcfg->lock);

	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out without hooking into any of
	 * the main system.  (We don't throw error here because it seems useful to
	 * allow the cs_* functions to be created even when the
	 * module isn't active.  The functions must protect themselves against
	 * being called then, however.)
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	elog(WARNING, "raftable init");

	for (i = 0; i < MAX_SERVERS; i++)
		wcfg.peers[i].up = false;

	wcfg.raft_config.peernum_max = MAX_SERVERS;

	RequestAddinShmemSpace(sizeof(WorkerConfig));
	PreviousShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = raftableShmemStartup;

	DefineCustomIntVariable("raftable.id",
		"Raft peer id of current instance", NULL,
		&wcfg.id, -1,
		-1, MAX_SERVERS-1,
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
		"Raft message length", NULL,
		&wcfg.raft_config.msg_len_max, 500,
		1, 500,
		PGC_POSTMASTER, 0, NULL, NULL, NULL
	);
}

void raftable_peer(int id, const char *host, int port)
{
	HostPort *hp;
	SpinLockAcquire(&sharedcfg->lock);
	hp = (HostPort *)sharedcfg->peers + id;
	hp->up = true;
	strncpy(hp->host, host, sizeof(hp->host));
	hp->port = port;
	SpinLockRelease(&sharedcfg->lock);
}

pid_t raftable_start(int id)
{
	BackgroundWorker worker;

	pid_t workerpid = -1;

	if ((id < 0) || (id >= MAX_SERVERS))
	{
		elog(ERROR, "raftable id %d is out of range", id);
		return -1;
	}

	if (!sharedcfg->peers[id].up)
	{
		elog(ERROR,
			 "cannot start raftable worker as id %d,"
			 " the id is not configured", id);
		return -1;
	}

	sharedcfg->id = id;

	snprintf(worker.bgw_name, BGW_MAXLEN, "raftable worker %d", sharedcfg->id);
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	worker.bgw_main = raftable_worker_main;
	worker.bgw_main_arg = PointerGetDatum(&sharedcfg);

	if (IsUnderPostmaster)
	{
		BackgroundWorkerHandle *handle;
		worker.bgw_notify_pid = MyProcPid;
		if (!RegisterDynamicBackgroundWorker(&worker, &handle))
			elog(ERROR, "cannot register dynamic background worker for raftable");
		if (WaitForBackgroundWorkerStartup(handle, &workerpid) != BGWH_STARTED)
			elog(ERROR, "dynamic background worker for raftable failed to start");
	}
	else
	{
		worker.bgw_notify_pid = 0;
		RegisterBackgroundWorker(&worker);
	}

	return workerpid;
}

void raftable_stop(void)
{
	elog(ERROR, "raftable_stop() not implemented");
}

void
_PG_fini(void)
{
	elog(WARNING, "raftable fini");
	shmem_startup_hook = PreviousShmemStartupHook;
}

/*
static void parse_peers(HostPort *peers, char *peerstr)
{
	char *state, *substate;
	char *peer, *s;
	char *host;
	int id, port;
	int i;
	peerstr = pstrdup(peerstr);

	for (i = 0; i < MAX_SERVERS; i++)
		peers[i].up = false;


	fprintf(stderr, "parsing '%s'\n", peerstr);
	peer = strtok_r(peerstr, ",", &state);
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

		Assert(!peers[id].up);
		peers[id].up = true;
		peers[id].port = port;
		strncpy(peers[id].host, host, sizeof(peers[id].host));

		peer = strtok_r(NULL, ",", &state);
	}

	pfree(peerstr);
}
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "raft.h"
#include "util.h"

#define bool raft_bool_t
#define true 1
#define false 0

#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 6543

typedef enum roles {
	FOLLOWER,
	CANDIDATE,
	LEADER
} role_t;

#define UDP_SAFE_SIZE 508

typedef struct raft_entry_t {
	int term;
	bool snapshot;
	raft_update_t update;
	int bytes;
} raft_entry_t;

typedef struct raft_log_t {
	int first;
	int size;    // number of entries past first
	int acked;   // number of entries fully replicated to the majority of peers
	int applied; // number of entries applied to the state machine
	raft_entry_t *entries; // wraps around
	raft_entry_t newentry; // partially received entry
} raft_log_t;

typedef struct raft_progress_t {
	int entries; // number of entries fully sent/acked
	int bytes;   // number of bytes of the currently being sent entry sent/acked
} raft_progress_t;

typedef struct raft_peer_t {
	bool up;

	int seqno;  // the rpc sequence number
	raft_progress_t acked; // the number of entries:bytes acked by this peer
	int applied; // the number of entries applied by this peer

	char *host;
	int port;
	struct sockaddr_in addr;

	int silent_ms; // how long was this peer silent
} raft_peer_t;

typedef struct raft_data_t {
	int term;   // current term (latest term we have seen)
	int vote;   // who received our vote in current term
	role_t role;
	int me;     // my id
	int votes;  // how many votes are for me (if candidate)
	int leader; // the id of the leader
	raft_log_t log;

	int sock;

	int peernum;
	raft_peer_t *peers;

	int timer;

	raft_config_t config;
} raft_data_t;

#define RAFT_LOG(RAFT, INDEX) ((RAFT)->log.entries[(INDEX) % (RAFT)->config.log_len])
#define RAFT_LOG_FIRST_INDEX(RAFT) ((RAFT)->log.first)
#define RAFT_LOG_LAST_INDEX(RAFT) ((RAFT)->log.first + (RAFT)->log.size - 1)

#define RAFT_MSG_UPDATE 0 // append entry
#define RAFT_MSG_DONE   1 // entry appended
#define RAFT_MSG_CLAIM  2 // vote for me
#define RAFT_MSG_VOTE   3 // my vote

typedef struct raft_msg_data_t {
	int msgtype;
	int curterm;
	int from;
	int seqno;
} raft_msg_data_t;

typedef struct raft_msg_update_t {
	raft_msg_data_t msg;
	bool snapshot; // true if this message contains a snapshot
	int previndex; // the index of the preceding log entry
	int prevterm;  // the term of the preceding log entry

	bool empty;    // the message is just a heartbeat if empty

	int entryterm;
	int totallen;

	int acked;     // the leader's acked number

	int offset;
	int len;
	char data[1];
} raft_msg_update_t;

typedef struct raft_msg_done_t {
	raft_msg_data_t msg;
	int entryterm;  // the term of the appended entry
	raft_progress_t progress; // the progress after appending
	int applied;
	bool success;
	// the message is considered acked when the last chunk appends successfully
} raft_msg_done_t;

typedef struct raft_msg_claim_t {
	raft_msg_data_t msg;
	int index; // the index of my last completely received entry
	int lastterm;  // the term of my last entry
} raft_msg_claim_t;

typedef struct raft_msg_vote_t {
	raft_msg_data_t msg;
	bool granted;
} raft_msg_vote_t;

typedef union {
	raft_msg_update_t u;
	raft_msg_done_t d;
	raft_msg_claim_t c;
	raft_msg_vote_t v;
} raft_msg_any_t;

static bool raft_config_is_ok(raft_config_t *config) {
	bool ok = true;

	if (config->peernum_max < 3) {
		shout("please ensure peernum_max >= 3\n");
		ok = false;
	}

	if (config->heartbeat_ms >= config->election_ms_min) {
		shout("please ensure heartbeat_ms < election_ms_min (substantially)\n");
		ok = false;
	}

	if (config->election_ms_min >= config->election_ms_max) {
		shout("please ensure election_ms_min < election_ms_max\n");
		ok = false;
	}

	if (sizeof(raft_msg_update_t) + config->chunk_len - 1 > UDP_SAFE_SIZE) {
		shout(
			"please ensure chunk_len <= %lu, %d is too much for UDP\n",
			UDP_SAFE_SIZE - sizeof(raft_msg_update_t) + 1,
			config->chunk_len
		);
		ok = false;
	}

	if (config->msg_len_max < sizeof(raft_msg_any_t)) {
		shout("please ensure msg_len_max >= %lu\n", sizeof(raft_msg_any_t));
		ok = false;
	}

	return ok;
}

static void reset_progress(raft_progress_t *p) {
	p->entries = 0;
	p->bytes = 0;
}

static void raft_peer_init(raft_peer_t *p) {
	p->up = false;
	p->seqno = 0;
	reset_progress(&p->acked);
	p->applied = 0;

	p->host = DEFAULT_LISTENHOST;
	p->port = DEFAULT_LISTENPORT;
	p->silent_ms = 0;
}

static void raft_entry_init(raft_entry_t *e) {
	e->term = 0;
	e->snapshot = false;
	e->update.len = 0;
	e->update.data = NULL;
	e->update.userdata = NULL;
	e->bytes = 0;
}

static bool raft_log_init(raft_t raft) {
	raft_log_t *l = &raft->log;
	int i;
	l->first = 0;
	l->size = 0;
	l->acked = 0;
	l->applied = 0;
	l->entries = malloc(raft->config.log_len * sizeof(raft_entry_t));
	if (!l->entries) {
		shout("failed to allocate memory for raft log\n");
		return false;
	}
	for (i = 0; i < raft->config.log_len; i++) {
		raft_entry_init(l->entries + i);
	}
	raft_entry_init(&l->newentry);
	return true;
}

static bool raft_peers_init(raft_t raft) {
	int i;
	raft->peers = malloc(raft->config.peernum_max * sizeof(raft_peer_t));
	if (!raft->peers) {
		shout("failed to allocate memory for raft peers\n");
		return false;
	}
	for (i = 0; i < raft->config.peernum_max; i++) {
		raft_peer_init(raft->peers + i);
	}
	return true;
}

raft_t raft_init(raft_config_t *config) {
	raft_t raft = NULL;

	if (!raft_config_is_ok(config)) {
		goto cleanup;
	}

	raft = malloc(sizeof(raft_data_t));
	if (!raft) {
		shout("failed to allocate memory for raft instance\n");
		goto cleanup;
	}
	raft->log.entries = NULL;
	raft->peers = NULL;

	memcpy(&raft->config, config, sizeof(raft_config_t));
	raft->sock = -1;
	raft->term = 0;
	raft->vote = NOBODY;
	raft->role = FOLLOWER;
	raft->votes = 0;
	raft->me = NOBODY;
	raft->leader = NOBODY;
	raft->peernum = 0;

	if (!raft_log_init(raft)) goto cleanup;
	if (!raft_peers_init(raft)) goto cleanup;

	return raft;

cleanup:
	if (raft) {
		free(raft->peers);
		free(raft->log.entries);
		free(raft);
	}
	return NULL;
}

static void raft_reset_timer(raft_t r) {
	if (r->role == LEADER) {
		r->timer = r->config.heartbeat_ms;
	} else {
		r->timer = rand_between(
			r->config.election_ms_min,
			r->config.election_ms_max
		);
	}
}

bool raft_peer_up(raft_t r, int id, char *host, int port, bool self) {
	raft_peer_t *p = r->peers + id;
	struct addrinfo hint;
	struct addrinfo *a = NULL;
	char portstr[6];

	if (r->peernum >= r->config.peernum_max) {
		shout("too many peers\n");
		return false;
	}

	raft_peer_init(p);
	p->up = true;
	p->host = host;
	p->port = port;

	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_family = AF_INET;
	hint.ai_protocol = getprotobyname("udp")->p_proto;

	snprintf(portstr, 6, "%d", port);

	if (getaddrinfo(host, portstr, &hint, &a))
	{
		shout(
			"cannot convert the host string '%s'"
			" to a valid address: %s\n", host, strerror(errno));
		return false;
	}
    
    assert(a != NULL && a->ai_addrlen <= sizeof(p->addr));
	memcpy(&p->addr, a->ai_addr, a->ai_addrlen);

	if (self) {
		if (r->me != NOBODY) {
			shout("cannot set 'self' peer multiple times\n");
			return false;
		}
		r->me = id;
		srand(id);
		raft_reset_timer(r);
	}
	r->peernum++;
	return true;
}

static int raft_apply(raft_t raft) {
	int applied_now = 0;
	raft_log_t *l = &raft->log;
	while ((l->applied < l->acked) && (l->applied <= RAFT_LOG_LAST_INDEX(raft))) {
		raft_entry_t *e = &RAFT_LOG(raft, l->applied);
		assert(e->update.len == e->bytes);
		raft->config.applier(raft->config.userdata, e->update, false);
		raft->log.applied++;
		applied_now++;
	}
	return applied_now;
}

static void socket_set_recv_timeout(int sock, int ms) {
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = ((ms % 1000) * 1000);
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
		shout("failed to set socket recv timeout: %s\n", strerror(errno));
	}
}

static void socket_set_reuseaddr(int sock) {
	int optval = 1;
	if (setsockopt(
		sock, SOL_SOCKET, SO_REUSEADDR,
		(char const*)&optval, sizeof(optval)
	) == -1) {
		shout("failed to set socket to reuseaddr: %s\n", strerror(errno));
	}
}

int raft_create_udp_socket(raft_t r) {
	assert(r->me != NOBODY);
	raft_peer_t *me = r->peers + r->me;
	struct addrinfo hint;
	struct addrinfo *addrs = NULL;
	struct addrinfo *a;
	char portstr[6];
	int rc;
	memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_family = AF_INET;
	hint.ai_protocol = getprotobyname("udp")->p_proto;

	snprintf(portstr, 6, "%d", me->port);

	if ((rc = getaddrinfo(me->host, portstr, &hint, &addrs)) != 0)
	{
		shout(
			"cannot convert the host string '%s'"
			" to a valid address: %s\n", me->host, gai_strerror(rc));
		return -1;
	}

	for (a = addrs; a != NULL; a = a->ai_next)
	{
		int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sock < 0) {
			shout("cannot create socket: %s\n", strerror(errno));
			continue;
		}
		socket_set_reuseaddr(sock);
		socket_set_recv_timeout(sock, r->config.heartbeat_ms);

		debug("binding udp %s:%d\n", me->host, me->port);
		if (bind(sock, a->ai_addr, a->ai_addrlen) < 0) {			
			shout("cannot bind the socket: %s\n", strerror(errno));
			close(sock);
			continue;
		}
		r->sock = sock;
		assert(a->ai_addrlen <= sizeof(me->addr));
		memcpy(&me->addr, a->ai_addr, a->ai_addrlen);
		return sock;
	}

	shout("cannot resolve the host string '%s' to a valid address\n",
		  me->host
		);
	return -1;
}

static bool msg_size_is(raft_msg_t m, int mlen) {
	switch (m->msgtype) {
		case RAFT_MSG_UPDATE:
			return mlen == sizeof(raft_msg_update_t) + ((raft_msg_update_t *)m)->len - 1;
		case RAFT_MSG_DONE:
			return mlen == sizeof(raft_msg_done_t);
		case RAFT_MSG_CLAIM:
			return mlen == sizeof(raft_msg_claim_t);
		case RAFT_MSG_VOTE:
			return mlen == sizeof(raft_msg_vote_t);
	}
	return false;
}

static void raft_send(raft_t r, int dst, void *m, int mlen) {
	assert(r->peers[dst].up);
	assert(mlen <= r->config.msg_len_max);
	assert(msg_size_is((raft_msg_t)m, mlen));
	assert(((raft_msg_t)m)->msgtype >= 0);
	assert(((raft_msg_t)m)->msgtype < 4);
	assert(dst >= 0);
	assert(dst < r->config.peernum_max);
	assert(dst != r->me);
	assert(((raft_msg_t)m)->from == r->me);

	raft_peer_t *peer = r->peers + dst;

	int sent = sendto(
		r->sock, m, mlen, 0,
		(struct sockaddr*)&peer->addr, sizeof(peer->addr)
	);
	if (sent == -1) {
		shout(
			"failed to send a msg to [%d]: %s\n",
			dst, strerror(errno)
		);
	}
}

static void raft_beat(raft_t r, int dst) {
	if (dst == NOBODY) {
		// send a beat/update to everybody
		int i;
		for (i = 0; i < r->config.peernum_max; i++) {
			if (!r->peers[i].up) continue;
			if (i == r->me) continue;
			raft_beat(r, i);
		}
		return;
	}

	assert(r->role == LEADER);
	assert(r->leader == r->me);

	raft_peer_t *p = r->peers + dst;

	raft_msg_update_t *m = malloc(sizeof(raft_msg_update_t) + r->config.chunk_len - 1);

	m->msg.msgtype = RAFT_MSG_UPDATE;
	m->msg.curterm = r->term;
	m->msg.from = r->me;

	if (p->acked.entries <= RAFT_LOG_LAST_INDEX(r)) {
		int sendindex;

		debug("%d has acked %d:%d\n", dst, p->acked.entries, p->acked.bytes);

		if (p->acked.entries < RAFT_LOG_FIRST_INDEX(r)) {
			// The peer has woken up from anabiosis. Send the first
			// log entry (which is usually a snapshot).
			debug("sending the snapshot to %d\n", dst);
			sendindex = RAFT_LOG_FIRST_INDEX(r);
			assert(RAFT_LOG(r, sendindex).snapshot);
		} else {
			// The peer is a bit behind. Send an update.
			debug("sending update %d snapshot to %d\n", p->acked.entries, dst);
			sendindex = p->acked.entries;
		}
		m->snapshot = RAFT_LOG(r, sendindex).snapshot;
		debug("will send index %d to %d\n", sendindex, dst);

		m->previndex = sendindex - 1;
		raft_entry_t *e = &RAFT_LOG(r, sendindex);

		if (m->previndex >= 0) {
			m->prevterm = RAFT_LOG(r, m->previndex).term;
		} else {
			m->prevterm = -1;
		}
		m->entryterm = e->term;
		m->totallen = e->update.len;
		m->empty = false;
		m->offset = p->acked.bytes;
		m->len = min(r->config.chunk_len, m->totallen - m->offset);
		assert(m->len > 0);
		memcpy(m->data, e->update.data + m->offset, m->len);
	} else {
		// The peer is up to date. Send an empty heartbeat.
		debug("sending empty heartbeat to %d\n", dst);
		m->empty = true;
		m->len = 0;
	}
	m->acked = r->log.acked;

	p->seqno++;
	m->msg.seqno = p->seqno;
	if (!m->empty) {
		debug(
			"sending seqno=%d to %d: offset=%d size=%d total=%d term=%d snapshot=%s\n",
			m->msg.seqno, dst, m->offset, m->len, m->totallen, m->entryterm, m->snapshot ? "true" : "false"
		);
	} else {
		debug("sending seqno=%d to %d: heartbeat\n", m->msg.seqno, dst);
	}

	raft_send(r, dst, m, sizeof(raft_msg_update_t) + m->len - 1);
	free(m);
}

static void raft_reset_bytes_acked(raft_t r) {
	int i;
	for (i = 0; i < r->config.peernum_max; i++) {
		r->peers[i].acked.bytes = 0;
	}
}

static void raft_reset_silent_time(raft_t r, int id) {
	int i;
	for (i = 0; i < r->config.peernum_max; i++) {
		if ((i == id) || (id == NOBODY)) {
			r->peers[i].silent_ms = 0;
		}
	}
}

// Returns true if we got the support of a majority and became the leader
static bool raft_become_leader(raft_t r) {
	if (r->votes * 2 > r->peernum) {
		// got the support of a majority
		r->role = LEADER;
		r->leader = r->me;
		raft_reset_bytes_acked(r);
		raft_reset_silent_time(r, NOBODY);
		raft_reset_timer(r);
		shout("became the leader\n");
		return true;
	}
	return false;
}

static void raft_claim(raft_t r) {
	assert(r->role == CANDIDATE);
	assert(r->leader == NOBODY);

	r->votes = 1; // vote for self
	if (raft_become_leader(r)) {
		// no need to send any messages, since we are alone
		return;
	}

	raft_msg_claim_t m;

	m.msg.msgtype = RAFT_MSG_CLAIM;
	m.msg.curterm = r->term;
	m.msg.from = r->me;

	m.index = r->log.first + r->log.size - 1;
	if (m.index >= 0) {
		m.lastterm = RAFT_LOG(r, m.index).term;
	} else {
		m.lastterm = -1;
	}

	int i;
	for (i = 0; i < r->config.peernum_max; i++) {
		if (!r->peers[i].up) continue;
		if (i == r->me) continue;
		raft_peer_t *s = r->peers + i;
		s->seqno++;
		m.msg.seqno = s->seqno;

		raft_send(r, i, &m, sizeof(m));
	}
}

static void raft_refresh_acked(raft_t r) {
	// pick each peer's acked and see if it is acked on the majority
	// TODO: count 'acked' inside the entry itself to remove the nested loop here
	int i, j;
	for (i = 0; i < r->config.peernum_max; i++) {
		raft_peer_t *p = r->peers + i;
		if (i == r->me) continue;
		if (!p->up) continue;

		int newacked = p->acked.entries;
		if (newacked <= r->log.acked) continue;

		int replication = 1; // count self as yes
		for (j = 0; j < r->config.peernum_max; j++) {
			if (j == r->me) continue;

			raft_peer_t *pp = r->peers + j;
			if (pp->acked.entries >= newacked) {
				replication++;
			}
		}

		assert(replication <= r->peernum);

		if (replication * 2 > r->peernum) {
			debug("===== GLOBAL PROGRESS: %d\n", newacked);
			r->log.acked = newacked;
		}
	}

	int applied = raft_apply(r);
	if (applied) {
		debug("applied %d updates\n", applied);
	}
}

static int raft_increase_silent_time(raft_t r, int ms) {
	int recent_peers = 1; // count myself as recent
	int i;
	for (i = 0; i < r->config.peernum_max; i++) {
		if (!r->peers[i].up) continue;
		if (i == r->me) continue;

		r->peers[i].silent_ms += ms;
		if (r->peers[i].silent_ms < r->config.election_ms_max) {
			recent_peers++;
		}
	}

	return recent_peers;
}

void raft_tick(raft_t r, int msec) {
	r->timer -= msec;
	if (r->timer < 0) {
		switch (r->role) {
			case FOLLOWER:
				shout(
					"lost the leader,"
					" claiming leadership\n"
				);
				r->leader = NOBODY;
				r->role = CANDIDATE;
				r->term++;
				raft_claim(r);
				break;
			case CANDIDATE:
				shout(
					"the vote failed,"
					" claiming leadership\n"
				);
				r->term++;
				raft_claim(r);
				break;
			case LEADER:
				raft_beat(r, NOBODY);
				break;
		}
		raft_reset_timer(r);
	}
	raft_refresh_acked(r);

	int recent_peers = raft_increase_silent_time(r, msec);
	if ((r->role == LEADER) && (recent_peers * 2 <= r->peernum)) {
		shout("lost quorum, demoting\n");
		r->leader = NOBODY;
		r->role = FOLLOWER;
	}
}

static int raft_compact(raft_t raft) {
	raft_log_t *l = &raft->log;
	int i;
	int compacted = 0;
	for (i = l->first; i < l->applied; i++) {
		raft_entry_t *e = &RAFT_LOG(raft, i);

		e->snapshot = false;
		free(e->update.data);
		e->update.len = 0;
		e->update.data = NULL;

		compacted++;
	}
	if (compacted) {
		l->first += compacted - 1;
		l->size -= compacted - 1;
		raft_entry_t *e = &RAFT_LOG(raft, RAFT_LOG_FIRST_INDEX(raft));
		e->update = raft->config.snapshooter(raft->config.userdata);
		e->bytes = e->update.len;
		e->snapshot = true;
		assert(l->first == l->applied - 1);

		// reset bytes progress of peers that were receiving the compacted entries
		for (i = 0; i < raft->config.peernum_max; i++) {
			raft_peer_t *p = raft->peers + i;
			if (!p->up) continue;
			if (i == raft->me) continue;
			if (p->acked.entries + 1 <= l->first)
				p->acked.bytes = 0;
		}
	}
	return compacted;
}

int raft_emit(raft_t r, raft_update_t update) {
	assert(r->leader == r->me);
	assert(r->role == LEADER);

	if (r->log.size == r->config.log_len) {
		int compacted = raft_compact(r);
		if (compacted > 1) {
			debug("compacted %d entries\n", compacted);
		} else {
			shout(
				"cannot emit new entries, the log is"
				" full and cannot be compacted\n"
			);
			return -1;
		}
	}

	int newindex = RAFT_LOG_LAST_INDEX(r) + 1;
	raft_entry_t *e = &RAFT_LOG(r, newindex);
	e->term = r->term;
	assert(e->update.len == 0);
	assert(e->update.data == NULL);
	e->update.len = update.len;
	e->bytes = update.len;
	e->update.data = malloc(update.len);
	memcpy(e->update.data, update.data, update.len);
	r->log.size++;

	raft_beat(r, NOBODY);
	raft_reset_timer(r);
	return newindex;
}

bool raft_applied(raft_t r, int id, int index) {
	if (r->me == id) {
		return r->log.applied > index;
	} else {
		raft_peer_t *p = r->peers + id;
		if (!p->up) return false;
		return p->applied > index;
	}
}

static bool raft_restore(raft_t r, int previndex, raft_entry_t *e) {
	int i;
	assert(e->bytes == e->update.len);
	assert(e->snapshot);
	for (i = RAFT_LOG_FIRST_INDEX(r); i <= RAFT_LOG_LAST_INDEX(r); i++) {
		raft_entry_t *victim = &RAFT_LOG(r, i);
		free(victim->update.data);
		victim->update.len = 0;
		victim->update.data = NULL;
	}
	int index = previndex + 1;
	r->log.first = index;
	r->log.size = 1;
	RAFT_LOG(r, index) = *e;
	raft_entry_init(e);

	r->config.applier(r->config.userdata, RAFT_LOG(r, index).update, true);
	r->log.applied = index + 1;
	return true;
}

static bool raft_appendable(raft_t r, int previndex, int prevterm) {
	int low, high;

	low = RAFT_LOG_FIRST_INDEX(r);
	if (low == 0) low = -1; // allow appending at the start
	high = RAFT_LOG_LAST_INDEX(r);

	if (!inrange(low, previndex, high))
	{
		debug(
			"previndex %d is outside log range %d-%d\n",
			previndex, low, high
		);
		return false;
	}

	if (previndex != -1) {
		raft_entry_t *pe = &RAFT_LOG(r, previndex);
		if (pe->term != prevterm) {
			debug("log term %d != prevterm %d\n", pe->term, prevterm);
			return false;
		}
	}

	return true;
}

static bool raft_append(raft_t r, int previndex, int prevterm, raft_entry_t *e) {
	assert(e->bytes == e->update.len);
	assert(!e->snapshot);

	raft_log_t *l = &r->log;

	debug(
		"log_append(%p, previndex=%d, prevterm=%d,"
		" term=%d)\n",
		(void *)l, previndex, prevterm,
		e->term
	);

	if (!raft_appendable(r, previndex, prevterm)) return false;

	if (previndex == RAFT_LOG_LAST_INDEX(r)) {
		debug("previndex == last\n");
		// appending to the end
		// check if the log can accomodate
		if (l->size == r->config.log_len) {
			debug("log is full\n");
			int compacted = raft_compact(r);
			if (compacted) {
				debug("compacted %d entries\n", compacted);
			} else {
				return false;
			}
		}
	}

	int index = previndex + 1;
	raft_entry_t *slot = &RAFT_LOG(r, index);

	if (index < l->first + l->size) {
		// replacing an existing entry
		if (slot->term != e->term) {
			// entry conflict, remove the entry and all that follow
			l->size = index - l->first;
		}
		assert(slot->update.data);
		free(slot->update.data);
	}

	if (index == l->first + l->size) {
		l->size++;
	}
	*slot = *e;
	raft_entry_init(e);

	return true;
}

static void raft_handle_update(raft_t r, raft_msg_update_t *m) {
	int sender = m->msg.from;

	raft_msg_done_t reply;
	reply.msg.msgtype = RAFT_MSG_DONE;
	reply.msg.curterm = r->term;
	reply.msg.from = r->me;
	reply.msg.seqno = m->msg.seqno;

	raft_entry_t *e = &r->log.newentry;
	raft_update_t *u = &e->update;

	if (!m->empty && !m->snapshot && !raft_appendable(r, m->previndex, m->prevterm)) goto finish;

	if (RAFT_LOG_LAST_INDEX(r) >= 0) {
		reply.entryterm = RAFT_LOG(r, RAFT_LOG_LAST_INDEX(r)).term;
	} else {
		reply.entryterm = -1;
	}
	reply.success = false;

	// the message is too old
	if (m->msg.curterm < r->term) {
		debug("refuse old message %d < %d\n", m->msg.curterm, r->term);
		goto finish;
	}

	if (sender != r->leader) {
		shout("changing leader to %d\n", sender);
		r->leader = sender;
	}

	r->peers[sender].silent_ms = 0;
	raft_reset_timer(r);

	if (m->acked > r->log.acked) {
		r->log.acked = min(
			r->log.first + r->log.size,
			m->acked
		);
		raft_peer_t *p = r->peers + sender;
		p->acked.entries = r->log.acked;
		p->acked.bytes = 0;
	}

	if (!m->empty) {
		debug(
			"got a chunk seqno=%d from %d: offset=%d size=%d total=%d term=%d snapshot=%s\n",
			m->msg.seqno, sender, m->offset, m->len, m->totallen, m->entryterm, m->snapshot ? "true" : "false"
		);

		if ((m->offset > 0) && (e->term != m->entryterm)) {
			shout("a chunk of another version of the entry received, resetting progress to avoid corruption\n");
			e->term = m->entryterm;
			e->bytes = 0;
			goto finish;
		}

		if (m->offset > e->bytes) {
			shout("unexpectedly large offset %d for a chunk, ignoring to avoid gaps\n", m->offset);
			goto finish;
		}

		u->len = m->totallen;
		u->data = realloc(u->data, m->totallen);

		memcpy(u->data + m->offset, m->data, m->len);
		e->term = m->entryterm;
		e->bytes = m->offset + m->len;
		assert(e->bytes <= u->len);

		e->snapshot = m->snapshot;

		if (e->bytes == u->len) {
			if (m->snapshot) {
				if (!raft_restore(r, m->previndex, e)) {
					shout("restore from snapshot failed\n");
					goto finish;
				}
			} else {
				if (!raft_append(r, m->previndex, m->prevterm, e)) {
					debug("log_append failed\n");
					goto finish;
				}
			}
		}
	} else {
		// just a heartbeat
		e->bytes = 0;
	}

	if (RAFT_LOG_LAST_INDEX(r) >= 0) {
		reply.entryterm = RAFT_LOG(r, RAFT_LOG_LAST_INDEX(r)).term;
	} else {
		reply.entryterm = -1;
	}
	reply.applied = r->log.applied;

	reply.success = true;
finish:
	reply.progress.entries = RAFT_LOG_LAST_INDEX(r) + 1;
	reply.progress.bytes = e->bytes;

	debug(
		"replying with %s to %d, our progress is %d:%d\n",
		reply.success ? "ok" : "reject",
		sender,
		reply.progress.entries,
		reply.progress.bytes
	);
	raft_send(r, sender, &reply, sizeof(reply));
}

static void raft_handle_done(raft_t r, raft_msg_done_t *m) {
	if (r->role != LEADER) {
		return;
	}

	int sender = m->msg.from;
	if (sender == r->me) {
		return;
	}

	raft_peer_t *peer = r->peers + sender;
	if (m->msg.seqno != peer->seqno) {
		debug("[from %d] ============= mseqno(%d) != sseqno(%d)\n", sender, m->msg.seqno, peer->seqno);
		return;
	}
	peer->seqno++;
	if (m->msg.curterm < r->term) {
		debug("[from %d] ============= msgterm(%d) != term(%d)\n", sender, m->msg.curterm, r->term);
		return;
	}

	peer->applied = m->applied;

	if (m->success) {
		debug("[from %d] ============= done (%d, %d)\n", sender, m->progress.entries, m->progress.bytes);
		peer->acked = m->progress;
		peer->silent_ms = 0;
	} else {
		debug("[from %d] ============= refused\n", sender);
		if (peer->acked.entries > 0) {
			peer->acked.entries--;
			peer->acked.bytes = 0;
		}
	}

	if (peer->acked.entries <= RAFT_LOG_LAST_INDEX(r)) {
		// send the next entry
		raft_beat(r, sender);
	}
}

static void raft_set_term(raft_t r, int term) {
	assert(term > r->term);
	r->term = term;
	r->vote = NOBODY;
	r->votes = 0;
}

void raft_ensure_term(raft_t r, int term) {
	assert(r->role == LEADER);
	if (term > r->term) {
		r->term = term;
	}
}

static void raft_handle_claim(raft_t r, raft_msg_claim_t *m) {
	int candidate = m->msg.from;

	if (m->msg.curterm >= r->term) {
		if (r->role != FOLLOWER) {
			shout("there is another candidate, demoting myself\n");
		}
		if (m->msg.curterm > r->term) {
			raft_set_term(r, m->msg.curterm);
		}
		r->role = FOLLOWER;
	}

	raft_msg_vote_t reply;
	reply.msg.msgtype = RAFT_MSG_VOTE;
	reply.msg.curterm = r->term;
	reply.msg.from = r->me;
	reply.msg.seqno = m->msg.seqno;

	reply.granted = false;

	if (m->msg.curterm < r->term) goto finish;

	// check if the candidate's log is up to date
	if (m->index < r->log.first + r->log.size - 1) goto finish;
	if (m->index == r->log.first + r->log.size - 1) {
		if ((m->index >= 0) && (RAFT_LOG(r, m->index).term != m->lastterm)) {
			goto finish;
		}
	}

	if ((r->vote == NOBODY) || (r->vote == candidate)) {
		r->vote = candidate;
		raft_reset_timer(r);
		reply.granted = true;
	}
finish:
	shout("voting %s %d\n", reply.granted ? "for" : "against", candidate);
	raft_send(r, candidate, &reply, sizeof(reply));
}

static void raft_handle_vote(raft_t r, raft_msg_vote_t *m) {
	int sender = m->msg.from;
	raft_peer_t *peer = r->peers + sender;
	if (m->msg.seqno != peer->seqno) return;
	peer->seqno++;
	if (m->msg.curterm < r->term) return;

	if (r->role != CANDIDATE) return;

	if (m->granted) {
		r->votes++;
	}

	raft_become_leader(r);
}

void raft_handle_message(raft_t r, raft_msg_t m) {
	if (m->curterm > r->term) {
		if (r->role != FOLLOWER) {
			shout("I have an old term, demoting myself\n");
		}
		raft_set_term(r, m->curterm);
		r->role = FOLLOWER;
	}

	assert(m->msgtype >= 0);
	assert(m->msgtype < 4);
	switch (m->msgtype) {
		case RAFT_MSG_UPDATE:
			raft_handle_update(r, (raft_msg_update_t *)m);
			break;
		case RAFT_MSG_DONE:
			raft_handle_done(r, (raft_msg_done_t *)m);
			break;
		case RAFT_MSG_CLAIM:
			raft_handle_claim(r, (raft_msg_claim_t *)m);
			break;
		case RAFT_MSG_VOTE:
			raft_handle_vote(r, (raft_msg_vote_t *)m);
			break;
		default:
			shout("unknown message type\n");
	}
}

static char buf[UDP_SAFE_SIZE];

raft_msg_t raft_recv_message(raft_t r) {
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(addr);

	//try to receive some data
	raft_msg_t m = (raft_msg_t)buf;
	int recved = recvfrom(
		r->sock, buf, sizeof(buf), 0,
		(struct sockaddr*)&addr, &addrlen
	);

	if (recved <= 0) {
		if (
			(errno == EAGAIN) ||
			(errno == EWOULDBLOCK) ||
			(errno == EINTR)
		) {
			return NULL;
		} else {
			shout("failed to recv: %s\n", strerror(errno));
			return NULL;
		}
	}

	if (!msg_size_is(m, recved)) {
		shout(
			"a corrupt msg recved from %s:%d\n",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port)
		);
		return NULL;
	}

	if ((m->from < 0) || (m->from >= r->config.peernum_max)) {
		shout(
			"the 'from' is out of range (%d)\n",
			m->from
		);
		return NULL;
	}

	if (m->from == r->me) {
		shout("the message is from myself O_o\n");
		return NULL;
	}

	raft_peer_t *peer = r->peers + m->from;
	if (memcmp(&peer->addr.sin_addr, &addr.sin_addr, sizeof(struct in_addr))) {
		shout(
			"the message is from a wrong address %s = %d"
			" (expected from %s = %d)\n",
			inet_ntoa(peer->addr.sin_addr),
			peer->addr.sin_addr.s_addr,
			inet_ntoa(addr.sin_addr),
			addr.sin_addr.s_addr
		);
	}

	if (peer->addr.sin_port != addr.sin_port) {
		shout(
			"the message is from a wrong port %d"
			" (expected from %d)\n",
			ntohs(peer->addr.sin_port),
			ntohs(addr.sin_port)
		);
	}

	return m;
}

bool raft_is_leader(raft_t r) {
	return r->role == LEADER;
}

int raft_get_leader(raft_t r) {
	return r->leader;
}

int raft_progress(raft_t r) {
	return r->log.applied;
}

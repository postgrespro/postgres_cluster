#include <assert.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#include "raft.h"
#include "util.h"

static void raft_server_init(raft_server_t *s) {
	s->seqno = 0;
	s->tosend = 0;
	s->acked = 0;

	s->host = DEFAULT_LISTENHOST;
	s->port = DEFAULT_LISTENPORT;
}

static void raft_reset_timer(raft_t *r) {
	if (r->role == ROLE_LEADER) {
		r->timer = HEARTBEAT_TIMEOUT_MS;
	} else {
		r->timer = rand_between(
			ELECTION_TIMEOUT_MS_MIN,
			ELECTION_TIMEOUT_MS_MAX
		);
	}
}

bool raft_add_server(raft_t *r, char *host, int port) {
	if (r->servernum >= MAX_SERVERS) {
		shout("too many servers\n");
		return false;
	}

	raft_server_t *s = r->servers + r->servernum;

	raft_server_init(s);
	s->host = host;
	s->port = port;

	if (inet_aton(s->host, &s->addr.sin_addr) == 0) {
		shout(
			"cannot convert the host string '%s'"
			" to a valid address\n", s->host
		);
		return false;
	}
	s->addr.sin_port = htons(s->port);

	r->servernum++;
	return true;
}

bool raft_set_myid(raft_t *r, int myid) {
	if ((myid < 0) || (myid >= r->servernum)) {
		shout(
			"myid %d out of range [%d;%d]\n",
			myid, 0, r->servernum - 1
		);
		return false;
	}
	r->me = myid;
	srand(myid);
	raft_reset_timer(r);
	return true;
}

void raft_init(raft_t *r) {
	r->term = 0;
	r->vote = NOBODY;
	r->role = ROLE_FOLLOWER;
	r->votes = 0;
	r->me = NOBODY;
	r->leader = NOBODY;

	r->log.first = 0;
	r->log.size = 0;
	r->log.acked = 0;
	r->log.applied = 0;

	r->servernum = 0;
}

int raft_apply(raft_t *r, raft_applier_t applier) {
	int applied_now = 0;
	while (r->log.acked > r->log.applied) {
		applier(
			RAFT_LOG(r, r->log.applied).action,
			RAFT_LOG(r, r->log.applied).argument
		);
		r->log.applied++;
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

int raft_create_udp_socket(raft_t *r) {
	assert(r->me != NOBODY);
	raft_server_t *me = r->servers + r->me;

	r->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (r->sock == -1) {
		shout(
			"cannot create the listening"
			" socket: %s\n",
			strerror(errno)
		);
		return -1;
	}

	socket_set_reuseaddr(r->sock);
	socket_set_recv_timeout(r->sock, HEARTBEAT_TIMEOUT_MS);

	// zero out the structure
	memset((char*)&me->addr, 0, sizeof(me->addr));

	me->addr.sin_family = AF_INET;
	if (inet_aton(me->host, &me->addr.sin_addr) == 0) {
		shout(
			"cannot convert the host string"
			" '%s' to a valid address\n",
			me->host
		);
		return -1;
	}
	me->addr.sin_port = htons(me->port);
	debug("binding udp %s:%d\n", me->host, me->port);
	if (bind(r->sock, (struct sockaddr*)&me->addr, sizeof(me->addr)) == -1) {
		shout("cannot bind the socket: %s\n", strerror(errno));
		return -1;
	}

	return r->sock;
}

static bool msg_size_is(raft_msg_t *m, int mlen) {
	switch (m->msgtype) {
		case RAFT_MSG_UPDATE:
			return mlen == sizeof(raft_msg_update_t);
		case RAFT_MSG_DONE:
			return mlen == sizeof(raft_msg_done_t);
		case RAFT_MSG_CLAIM:
			return mlen == sizeof(raft_msg_claim_t);
		case RAFT_MSG_VOTE:
			return mlen == sizeof(raft_msg_vote_t);
	}
	return false;
}

static void raft_send(raft_t *r, int dst, void *m, int mlen) {
	assert(msg_size_is((raft_msg_t*)m, mlen));
	assert(((raft_msg_t*)m)->msgtype >= 0);
	assert(((raft_msg_t*)m)->msgtype < 4);
	assert(dst >= 0);
	assert(dst < r->servernum);
	assert(dst != r->me);
	assert(((raft_msg_t*)m)->from == r->me);

	raft_server_t *server = r->servers + dst;

	int sent = sendto(
		r->sock, m, mlen, 0,
		(struct sockaddr*)&server->addr, sizeof(server->addr)
	);
	if (sent == -1) {
		shout(
			"failed to send a msg to [%d]: %s\n",
			dst, strerror(errno)
		);
	}
}

static void raft_beat(raft_t *r, int dst) {
	if (dst == NOBODY) {
		// send a beat/update to everybody
		int i;
		for (i = 0; i < r->servernum; i++) {
			if (i == r->me) continue;
			raft_beat(r, i);
		}
		return;
	}

	assert(r->role == ROLE_LEADER);
	assert(r->leader == r->me);

	raft_server_t *s = r->servers + dst;

	raft_msg_update_t m;

	m.msg.msgtype = RAFT_MSG_UPDATE;
	m.msg.term = r->term;
	m.msg.from = r->me;

	if (s->tosend < r->log.first + r->log.size) {
		raft_entry_t *e = &RAFT_LOG(r, s->tosend);
		if (e->snapshot) {
			// TODO: implement snapshot sending
			shout("tosend = %d, first = %d, size = %d\n", s->tosend, r->log.first, r->log.size);
			assert(false); // snapshot sending not implemented
		}

		// the follower is a bit behind: send an update
		m.previndex = s->tosend - 1;
		if (m.previndex >= 0) {
			m.prevterm = RAFT_LOG(r, m.previndex).term;
		} else {
			m.prevterm = -1;
		}
		m.entry = *e;
		m.empty = false;
	} else {
		// the follower is up to date: send a heartbeat
		m.empty = true;
	}
	m.acked = r->log.acked;

	s->seqno++;
	m.msg.seqno = s->seqno;
	if (!m.empty) {
		debug("[to %d] update with seqno = %d, tosend = %d, previndex = %d\n", dst, m.msg.seqno, s->tosend, m.previndex);
	}

	raft_send(r, dst, &m, sizeof(m));
}

static void raft_claim(raft_t *r) {
	assert(r->role == ROLE_CANDIDATE);
	assert(r->leader == NOBODY);

	r->votes = 1; // vote for self

	raft_msg_claim_t m;

	m.msg.msgtype = RAFT_MSG_CLAIM;
	m.msg.term = r->term;
	m.msg.from = r->me;

	m.index = r->log.first + r->log.size - 1;
	if (m.index >= 0) {
		m.term = RAFT_LOG(r, m.index).term;
	} else {
		m.term = -1;
	}

	int i;
	for (i = 0; i < r->servernum; i++) {
		if (i == r->me) continue;
		raft_server_t *s = r->servers + i;
		s->seqno++;
		m.msg.seqno = s->seqno;

		raft_send(r, i, &m, sizeof(m));
	}
}

void raft_tick(raft_t *r, int msec) {
	r->timer -= msec;
	if (r->timer < 0) {
		switch (r->role) {
			case ROLE_FOLLOWER:
				debug(
					"lost the leader,"
					" claiming leadership\n"
				);
				r->leader = NOBODY;
				r->role = ROLE_CANDIDATE;
				r->term++;
				raft_claim(r);
				break;
			case ROLE_CANDIDATE:
				debug(
					"the vote failed,"
					" claiming leadership\n"
				);
				r->term++;
				raft_claim(r);
				break;
			case ROLE_LEADER:
				raft_beat(r, NOBODY);
				break;
		}
		raft_reset_timer(r);
	}
}

static int raft_log_compact(raft_log_t *l, int keep_applied) {
	raft_entry_t snap;
	snap.snapshot = true;
	snap.minarg = INT_MAX;
	snap.maxarg = INT_MIN;

	int compacted = 0;
	int i;
	for (i = l->first; i < l->applied - keep_applied; i++) {
		raft_entry_t *e = l->entries + (i % RAFT_LOGLEN);
		snap.term = e->term;
		if (e->snapshot) {
			snap.minarg = min(snap.minarg, e->minarg);
			snap.maxarg = max(snap.maxarg, e->maxarg);
		} else {
			snap.minarg = min(snap.minarg, e->argument);
			snap.maxarg = max(snap.maxarg, e->argument);
		}
		compacted++;
	}
	if (compacted) {
		l->first += compacted - 1;
		l->size -= compacted - 1;
		assert(l->first < l->applied);
		l->entries[l->first % RAFT_LOGLEN] = snap;
	}
	return compacted;
}

bool raft_emit(raft_t *r, int action, int argument) {
	assert(r->leader == r->me);
	assert(r->role == ROLE_LEADER);

	if (r->log.size == RAFT_LOGLEN) {
		int compacted = raft_log_compact(&r->log, RAFT_KEEP_APPLIED);
		if (compacted > 1) {
			shout("compacted %d entries\n", compacted);
		} else {
			shout(
				"cannot emit new entries, the log is"
				" full and cannot be compacted\n"
			);
			return false;
		}
	}

	raft_entry_t *e = &RAFT_LOG(r, r->log.first + r->log.size);
	e->snapshot = false;
	e->term = r->term;
	e->action = action;
	e->argument = argument;
	r->log.size++;

	raft_beat(r, NOBODY);
	raft_reset_timer(r);
	return true;
}

static bool log_append(raft_log_t *l, int previndex, int prevterm, raft_entry_t *e) {
	if (e->snapshot) {
		assert(false);
	}
	debug(
		"log_append(%p, previndex=%d, prevterm=%d,"
		" term=%d, action=%d, argument=%d)\n",
		l, previndex, prevterm,
		e->term, e->action, e->argument
	);
	if (previndex != -1) {
		if (previndex < l->first) {
			debug("previndex < first\n");
			return false;
		}
	}
	if (previndex >= l->first + l->size) {
		debug("previndex > last\n");
		return false;
	}
	if (previndex == l->first + l->size - 1) {
		debug("previndex == last\n");
		// appending to the end
		// check if the log can accomodate
		if (l->size == RAFT_LOGLEN) {
			debug("log is full\n");
			int compacted = raft_log_compact(l, RAFT_KEEP_APPLIED);
			if (compacted) {
				debug("compacted %d entries\n", compacted);
			} else {
				return false;
			}
		}
	}

	if (previndex != -1) {
		raft_entry_t *pe = l->entries + (previndex % RAFT_LOGLEN);
		if (pe->term != prevterm) {
			debug("log term %d != prevterm %d\n", pe->term, prevterm);
			return false;
		}
	}

	int index = previndex + 1;
	raft_entry_t *slot = l->entries + (index % RAFT_LOGLEN);

	if (index < l->first + l->size) {
		// replacing an existing entry
		if (slot->term != e->term) {
			// entry conflict, remove the entry and all that follow
			l->size = index - l->first;
		}
	}

	if (index == l->first + l->size) {
		l->size++;
	}
	*slot = *e;

	return true;
}

static void raft_handle_update(raft_t *r, raft_msg_update_t *m) {
	int sender = m->msg.from;

	raft_msg_done_t reply;
	reply.msg.msgtype = RAFT_MSG_DONE;
	reply.msg.term = r->term;
	reply.msg.from = r->me;
	reply.msg.seqno = m->msg.seqno;

	reply.index = r->log.first + r->log.size - 1;
	if (reply.index >= 0) {
		reply.term = RAFT_LOG(r, reply.index).term;
	} else {
		reply.term = -1;
	}
	reply.success = false;

	// the message is too old
	if (m->msg.term < r->term) {
		debug("refuse old message %d < %d\n", m->msg.term, r->term);
		goto finish;
	}

	if (sender != r->leader) {
		shout("changing leader to %d\n", sender);
		r->leader = sender;
	}

	raft_reset_timer(r);

	if (m->acked > r->log.acked) {
		r->log.acked = min(
			r->log.first + r->log.size,
			m->acked
		);
		raft_server_t *s = r->servers + sender;
		s->acked = s->tosend = r->log.acked;
	}

	if (m->empty) {
		// just a hearbeat
		return;
	}

	if (!log_append(&r->log, m->previndex, m->prevterm, &m->entry)) {
		debug("log_append failed\n");
		goto finish;
	}
	reply.index = r->log.first + r->log.size - 1;
	reply.term = RAFT_LOG(r, reply.index).term;

	reply.success = true;
finish:
	raft_send(r, sender, &reply, sizeof(reply));
}

static void raft_refresh_acked(raft_t *r) {
	// pick each server's acked and see if it is acked on the majority
	// TODO: count 'acked' inside the entry itself to remove the nested loop here
	int i, j;
	for (i = 0; i < r->servernum; i++) {
		if (i == r->me) continue;
		int newacked = r->servers[i].acked;
		if (newacked <= r->log.acked) continue;

		int replication = 1; // count self as yes
		for (j = 0; j < r->servernum; j++) {
			if (j == r->me) continue;

			raft_server_t *s = r->servers + j;
			if (s->acked >= newacked) {
				replication++;
			}
		}

		assert(replication <= r->servernum);

		if (replication * 2 > r->servernum) {
			#ifdef MAJORITY_IS_NOT_ENOUGH
			if (replication < r->servernum) continue;
			#endif
			r->log.acked = newacked;
		}
	}
}

static void raft_handle_done(raft_t *r, raft_msg_done_t *m) {
	if (r->role != ROLE_LEADER) {
		return;
	}

	int sender = m->msg.from;
	if (sender == r->me) {
		return;
	}

	raft_server_t *server = r->servers + sender;
	if (m->msg.seqno != server->seqno) {
		debug("[from %d] ============= mseqno(%d) != sseqno(%d)\n", sender, m->msg.seqno, server->seqno);
		return;
	}
	server->seqno++;
	if (m->msg.term < r->term) {
		debug("[from %d] ============= msgterm(%d) != term(%d)\n", sender, m->term, r->term);
		return;
	}

	if (m->success) {
		debug("[from %d] ============= done\n", sender);
		server->tosend++;
		server->acked = server->tosend;
		raft_refresh_acked(r);
	} else {
		debug("[from %d] ============= refused\n", sender);
		if (server->tosend > 0) {
			// the client should have specified the last index it had gotten
			if (server->tosend == m->index + 1) {
				shout(
					"[from %d] the last index I have is %d, but"
					" I still refuse, this should not happen\n",
					sender, m->index
				);
				assert(false);
			}
			server->tosend = m->index + 1;
		}
		assert(server->tosend >= server->acked); // FIXME: remove this, because 'tosend' is actually allowed to be less than 'acked' if the follower has restarted
	}

	if (server->tosend < r->log.first + r->log.size) {
		// send the next entry
		raft_beat(r, sender);
	}
}

static void raft_set_term(raft_t *r, int term) {
	assert(term > r->term);
	r->term = term;
	r->vote = NOBODY;
	r->votes = 0;
}

void raft_start_next_term(raft_t *r) {
	assert(r->role == ROLE_LEADER);
	r->term++;
}

static void raft_handle_claim(raft_t *r, raft_msg_claim_t *m) {
	int candidate = m->msg.from;

	if (m->msg.term >= r->term) {
		if (r->role != ROLE_FOLLOWER) {
			shout("demoting myself\n");
		}
		if (m->msg.term > r->term) {
			raft_set_term(r, m->term);
		}
		r->role = ROLE_FOLLOWER;
	}

	raft_msg_vote_t reply;
	reply.msg.msgtype = RAFT_MSG_VOTE;
	reply.msg.term = r->term;
	reply.msg.from = r->me;
	reply.msg.seqno = m->msg.seqno;

	reply.granted = false;

	if (m->msg.term < r->term) goto finish;

	// check if the candidate's log is up to date
	if (m->index < r->log.first + r->log.size - 1) goto finish;
	if (m->index == r->log.first + r->log.size - 1) {
		if ((m->index >= 0) && (RAFT_LOG(r, m->index).term != m->term)) {
			goto finish;
		}
	}

	if ((r->vote == NOBODY) || (r->vote == candidate)) {
		r->vote = candidate;
		raft_reset_timer(r);
		reply.granted = true;
	}
finish:
	raft_send(r, candidate, &reply, sizeof(reply));
}

static void raft_handle_vote(raft_t *r, raft_msg_vote_t *m) {
	int sender = m->msg.from;
	raft_server_t *server = r->servers + sender;
	if (m->msg.seqno != server->seqno) return;
	server->seqno++;
	if (m->msg.term < r->term) return;

	if (r->role != ROLE_CANDIDATE) return;

	if (m->granted) {
		r->votes++;
	}

	if (r->votes * 2 > r->servernum) {
		// got the support of a majority
		r->role = ROLE_LEADER;
		r->leader = r->me;
		raft_reset_timer(r);
	}
}

void raft_handle_message(raft_t *r, raft_msg_t *m) {
	if (m->term > r->term) {
		if (r->role != ROLE_FOLLOWER) {
			shout("demoting myself\n");
		}
		raft_set_term(r, m->term);
		r->role = ROLE_FOLLOWER;
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

raft_msg_t *raft_recv_message(raft_t *r) {
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(addr);

	//try to receive some data, this is a blocking call
	char buf[1024];
	raft_msg_t *m = (raft_msg_t *)buf;
	int recved = recvfrom(
		r->sock, buf, sizeof(buf), 0,
		(struct sockaddr*)&addr, &addrlen
	);

	if (recved == -1) {
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

	if ((m->from < 0) || (m->from >= r->servernum)) {
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

	raft_server_t *server = r->servers + m->from;
	if (memcmp(&server->addr.sin_addr, &addr.sin_addr, sizeof(struct in_addr))) {
		shout(
			"the message is from a wrong address %s = %d"
			" (expected from %s = %d)\n",
			inet_ntoa(server->addr.sin_addr),
			server->addr.sin_addr.s_addr,
			inet_ntoa(addr.sin_addr),
			addr.sin_addr.s_addr
		);
	}

	if (server->addr.sin_port != addr.sin_port) {
		shout(
			"the message is from a wrong port %d"
			" (expected from %d)\n",
			ntohs(server->addr.sin_port),
			ntohs(addr.sin_port)
		);
	}

	return m;
}

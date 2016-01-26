#ifndef RAFT_H
#define RAFT_H

#include <arpa/inet.h>
#include <stdbool.h>
#include "arbiterlimits.h"

#define NOBODY -1

#define MAJORITY_IS_NOT_ENOUGH // wait for unanimous ack for applying a new entry

#define DEFAULT_LISTENHOST "0.0.0.0"
#define DEFAULT_LISTENPORT 5431

#define ROLE_FOLLOWER 0
#define ROLE_CANDIDATE 1
#define ROLE_LEADER 2

#if RAFT_KEEP_APPLIED >= RAFT_LOGLEN
#error please ensure RAFT_KEEP_APPLIED < RAFT_LOGLEN
#endif

#if HEARTBEAT_TIMEOUT_MS >= ELECTION_TIMEOUT_MS_MIN
#error please ensure HEARTBEAT_TIMEOUT_MS < ELECTION_TIMEOUT_MS_MIN (considerably)
#endif

#if ELECTION_TIMEOUT_MS_MIN >= ELECTION_TIMEOUT_MS_MAX
#error please ensure ELECTION_TIMEOUT_MS_MIN < ELECTION_TIMEOUT_MS_MAX
#endif

// raft module does not care what you mean by action and argument
typedef struct raft_entry_t {
	int term;
	bool snapshot; // true if this is a snapshot entry
	union {
		struct { // snapshot == false
			int action;
			int argument;
		};
		struct { // snapshot == true
			int minarg;
			int maxarg;
		};
	};
} raft_entry_t;

typedef void (*raft_applier_t)(int action, int argument);

typedef struct raft_log_t {
	int first;
	int size;    // number of entries past first
	int acked;   // number of entries replicated to the majority of servers
	int applied; // number of entries applied to the state machine
	raft_entry_t entries[RAFT_LOGLEN]; // wraps around
} raft_log_t;

typedef struct raft_server_t {
	int seqno;  // the rpc sequence number
	int tosend; // index of the next entry to send
	int acked;  // index of the highest entry known to be replicated

	char *host;
	int port;
	struct sockaddr_in addr;
} raft_server_t;

typedef struct raft_t {
	int term;   // current term (latest term we have seen)
	int vote;   // who received our vote in current term
	int role;
	int me;     // my id
	int votes;  // how many votes are for me (if candidate)
	int leader; // the id of the leader
	raft_log_t log;

	int sock;

	int servernum;
	raft_server_t servers[MAX_SERVERS];

	int timer;

	raft_applier_t applier;
} raft_t;

#define RAFT_LOG(RAFT, INDEX) ((RAFT)->log.entries[(INDEX) % (RAFT_LOGLEN)])

#define RAFT_MSG_UPDATE 0 // append entry
#define RAFT_MSG_DONE   1 // entry appended
#define RAFT_MSG_CLAIM  2 // vote for me
#define RAFT_MSG_VOTE   3 // my vote

typedef struct raft_msg_t {
	int msgtype;
	int term;
	int from;
	int seqno;
} raft_msg_t;

typedef struct raft_msg_update_t {
	raft_msg_t msg;
	int previndex; // the index of the preceding log entry
	int prevterm;  // the term of the preceding log entry

	bool empty;    // the message is just a heartbeat if empty
	raft_entry_t entry;

	int acked;     // the leader's acked number
} raft_msg_update_t;

typedef struct raft_msg_done_t {
	raft_msg_t msg;
	int index; // the index of the appended entry
	int term;  // the term of the appended entry
	bool success;
} raft_msg_done_t;

typedef struct raft_msg_claim_t {
	raft_msg_t msg;
	int index; // the index of my last entry
	int term;  // the term of my last entry
} raft_msg_claim_t;

typedef struct raft_msg_vote_t {
	raft_msg_t msg;
	bool granted;
} raft_msg_vote_t;

// configuration
void raft_init(raft_t *r);
bool raft_add_server(raft_t *r, char *host, int port);
bool raft_set_myid(raft_t *r, int myid);

// log actions
bool raft_emit(raft_t *r, int action, int argument);
int raft_apply(raft_t *r, raft_applier_t applier);

// control
void raft_tick(raft_t *r, int msec);
void raft_handle_message(raft_t *r, raft_msg_t *m);
raft_msg_t *raft_recv_message(raft_t *r);
int raft_create_udp_socket(raft_t *r);
void raft_ensure_term(raft_t *r, int term);

#endif

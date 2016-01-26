#ifndef ARBITER_LIMITS_H
#define ARBITER_LIMITS_H

// how many xids are reserved per raft term
#define XIDS_PER_TERM  1000000

// start a new term when this number of xids is left
#define NEW_TERM_THRESHOLD 100000

#define MAX_TRANSACTIONS 4096
#define MAX_SNAPSHOTS_PER_TRANS 8

#define BUFFER_SIZE (64 * 1024)
#define LISTEN_QUEUE_SIZE 100
#define MAX_STREAMS 4096

#define MAX_SERVERS 16
#define HEARTBEAT_TIMEOUT_MS 20
#define ELECTION_TIMEOUT_MS_MIN 150
#define ELECTION_TIMEOUT_MS_MAX 300
#define RAFT_LOGLEN 1024
#define RAFT_KEEP_APPLIED 512 // how many applied entries to keep during compaction

#endif

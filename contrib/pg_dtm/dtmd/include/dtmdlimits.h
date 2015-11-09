#ifndef DTMD_LIMITS_H
#define DTMD_LIMITS_H

#define MAX_TRANSACTIONS 4096

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

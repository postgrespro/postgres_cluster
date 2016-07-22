#ifndef RAFTABLE_WORKER_H
#define RAFTABLE_WORKER_H

#include <limits.h>

#include "storage/spin.h"
#include "raft.h"

#define MAX_SERVERS 64

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

typedef struct HostPort {
	bool up;
	char host[HOST_NAME_MAX + 1];
	int port;
} HostPort;

typedef struct WorkerConfig {
	int id;
	raft_config_t raft_config;
	HostPort peers[MAX_SERVERS];
	slock_t lock;
} WorkerConfig;

void raftable_worker_main(Datum arg);

#endif

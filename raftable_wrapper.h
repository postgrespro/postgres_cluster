#ifndef __RAFTABLE_WRAPPER_H__
#define __RAFTABLE_WRAPPER_H__

typedef struct RaftableTimestamp { 
	time_t time;   /* local time at master */
	uint32 master; /* master node for this operation */
	uint32 psn;    /* RAFTABLE serial number */
} RaftableTimestamp;

/*
 * Get key value.
 * Returns NULL if key doesn't exist.
 * Value should be copied into caller's private memory using palloc.
 * If "size" is not NULL, then it is assigned size of value.
 * If "ts" is not NULL, then it is assigned timestamp of last update of this value
 * If RAFT master is not accessible, then depending non value of "nowait" parameter, this funciton should either block until RAFT quorum is reached, either report error.
 */
extern void* RaftableGet(char const* key, size_t* size, RaftableTimestamp* ts, bool nowait);

/*
 * Set new value for the specified key. IF value is NULL, then key should be deleted.
 * If RAFT master is not accessible, then depending non value of "nowait" parameter, this funciton should either block until RAFT quorum is reached, either report error.
 * Returns false if rafttable is in minority
 */
extern bool RaftableSet(char const* key, void const* value, size_t size, bool nowait);

/* 
 * If key doesn't exists or its value is not equal to the specified value then store this value and return true.
 * Otherwise do nothing and return false.
 * If RAFT master is not accessible, then depending non value of "nowait" parameter, this funciton should either block until RAFT quorum is reached, either report error.
 */
extern bool  RaftableCAS(char const* key, char const* value, bool nowait);

extern bool MtmUseRaftable;

#endif

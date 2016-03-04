#ifndef __PAXOS_H__
#define __PAXOS_H__

typedef struct PaxosTimestamp { 
	time_t time;   /* local time at master */
	uint32 master; /* master node for this operation */
	uint32 psn;    /* PAXOS serial number */
} PaxosTimestamp;

extern void* PaxosGet(char const* key, int* size, PaxosTimestamp* ts);
extern void  PaxosSet(char const* key, void const* value, int size);

#endif

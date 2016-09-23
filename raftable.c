#include <dlfcn.h>
#include "postgres.h"
#include "raftable.h"
#include "raftable_wrapper.h"
#include "multimaster.h"
#include "replication/message.h"
#include "replication/logical.h"
#include "replication/origin.h"
#include "storage/proc.h"


/*
 * Raftable function proxies
 */
void* RaftableGet(char const* key, size_t* size, RaftableTimestamp* ts, bool nowait)
{
	void *value;
	size_t vallen;
	if (!MtmUseRaftable) { 
		int nodeId;
		if (sscanf(key, "node-mask-%d", &nodeId) == 1) {
			if (size != NULL) {
				*size = sizeof(nodemask_t);
			}
			return &Mtm->nodes[nodeId-1].connectivityMask;
		} else if (sscanf(key, "lock-graph-%d", &nodeId) == 1) {
			int lockGraphSize;
			void* lockGraphData;
			MtmLockNode(nodeId + MtmMaxNodes, LW_SHARED);
			lockGraphSize = Mtm->nodes[nodeId-1].lockGraphUsed;
			lockGraphData = palloc(lockGraphSize);
			memcpy(lockGraphData, Mtm->nodes[nodeId-1].lockGraphData, lockGraphSize);
			if (size != NULL) {
				*size = lockGraphSize;
			}
			MtmUnlockNode(nodeId + MtmMaxNodes);
			return lockGraphData;
		}
		return NULL;
	}
	value = raftable_get(key, &vallen, MtmHeartbeatSendTimeout);
	if (size != NULL) {
		*size = vallen;
	}
	return value;
}


bool RaftableSet(char const* key, void const* value, size_t size, bool nowait)
{
	if (MtmUseRaftable) {
		int tries = MtmHeartbeatRecvTimeout/MtmHeartbeatSendTimeout;
		timestamp_t start, stop;
		start = MtmGetSystemTime();
		if (nowait) {
			raftable_set(key, value, size, 0);
		} else { 
			while (!raftable_set(key, value, size, MtmHeartbeatSendTimeout))
			{
				MtmCheckHeartbeat();
				if (tries-- <= 0)
				{
					elog(WARNING, "Failed to send data to raftable in %d msec", MtmHeartbeatRecvTimeout);
					return false;
				}
			}
		}
		stop = MtmGetSystemTime();
		if (stop > start + MSEC_TO_USEC(MtmHeartbeatSendTimeout)) { 
			MTM_LOG1("Raftable set nowait=%d takes %ld microseconds", nowait, stop - start);
		}
	} else { 
		if (strncmp(key, "lock", 4) == 0) {
			MTM_LOG1("Broadcast deadlock graph");
			Assert(replorigin_session_origin == InvalidRepOriginId);
			XLogFlush(LogLogicalMessage("L", value, size, false));
		}
	}
	return true;
}

bool RaftableCAS(char const* key, char const* value, bool nowait)
{
	if (!MtmUseRaftable) return false;

	Assert(false); /* not implemented */
	return false;
}

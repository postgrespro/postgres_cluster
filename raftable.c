#include <dlfcn.h>
#include "postgres.h"
#include "raftable.h"
#include "raftable_wrapper.h"
#include "multimaster.h"

/*
 * Raftable function proxies
 */
void* RaftableGet(char const* key, size_t* size, RaftableTimestamp* ts, bool nowait)
{
	void *value;
	size_t vallen;
	if (!MtmUseRaftable) { 
		return NULL;
	}
	value = raftable_get(key, &vallen, MtmHeartbeatSendTimeout);
	if (size != NULL) {
		*size = vallen;
	}
	return value;
}


void RaftableSet(char const* key, void const* value, size_t size, bool nowait)
{
	if (MtmUseRaftable) {
		int tries = 10;
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
					MTM_LOG1("RaftableSet nowait=%d, all attempts failed", nowait);
					break;
				}
			}
		}
		stop = MtmGetSystemTime();
		if (stop > start + MSEC_TO_USEC(MtmHeartbeatSendTimeout)) { 
			MTM_LOG1("Raftable set nowait=%d takes %ld microseconds", nowait, stop - start);
		}
	}		
}

bool RaftableCAS(char const* key, char const* value, bool nowait)
{
	if (!MtmUseRaftable) return false;

	Assert(false); /* not implemented */
	return false;
}

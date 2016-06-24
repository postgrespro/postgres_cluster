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
	if (!MtmUseRaftable) { 
		return NULL;
	}
	return raftable_get(key, size);
}


void RaftableSet(char const* key, void const* value, size_t size, bool nowait)
{
	if (MtmUseRaftable) {
		if (nowait) {
			raftable_set(key, value, size, 0);
		} else { 
			while (!raftable_set(key, value, size, MtmHeartbeatSendTimeout)) { 
				MtmCheckHeartbeat();
			}
		}
	}		
}

bool RaftableCAS(char const* key, char const* value, bool nowait)
{
	if (!MtmUseRaftable) return false;

	Assert(false); /* not implemented */
	return false;
}

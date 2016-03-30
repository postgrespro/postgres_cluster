#include <dlfcn.h>
#include "postgres.h"
#include "raftable.h"


static raftable_get_t raftable_get_impl;
static raftable_set_t raftable_set_impl;

static void RaftableResolve()
{
	if (raftable_get_impl == NULL) { 
		void* dll = dlopen(NULL, RTLD_NOW);
		raftable_get_impl = dlsym(dll, "raftable_get");
		raftable_set_impl = dlsym(dll, "raftable_set");
		Assert(raftable_get_impl != NULL && raftable_set_impl != NULL);
	}
}

/*
 * Raftable function proxies
 */
void* RaftableGet(char const* key, int* size, RaftableTimestamp* ts, bool nowait)
{
	if (!MtmUseRaftable) { 
		return NULL;
	}
	RaftableResolve();
	return (*raftable_get_impl)(key, size, nowait ? 0 : -1);
}


void RaftableSet(char const* key, void const* value, int size, bool nowait)
{
	if (MtmUseRaftable) { 
		RaftableResolve();
		(*raftable_set_impl)(key, value, size, nowait ? 0 : -1);
	}		
}

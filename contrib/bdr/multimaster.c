/*
 * multimaster.c
 *
 * Multimaster based on logical replication
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "postmaster/bgworker.h"
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/lmgr.h"
#include "storage/shmem.h"
#include "storage/ipc.h"
#include "access/transam.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/tqual.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "postmaster/autovacuum.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "utils/syscache.h"

#include "multimaster.h"

#define MM_SHMEM_SIZE (1024*1024)
#define MM_HASH_SIZE  1003

typedef struct
{
	LWLockId hashLock;
    int      nNodes;
} MMState;

typedef struct
{
    TransactionId xid;
    int count;
} LocalTransaction;

void _PG_init(void);
void _PG_fini(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(mm_start_replication);
PG_FUNCTION_INFO_V1(mm_stop_replication);

static void MMShmemStartup(void);
static void MMInitialize(void);

static shmem_startup_hook_type MMPrevShmemStartupHook;

static HTAB* MMLocalTrans;

static char* MMConnStrs;
static int   MMNodeId;
static int   MMNodes;
static MMState* MM;


static uint32 mm_xid_hash_fn(const void *key, Size keysize)
{
	return (uint32)*(TransactionId*)key;
}

static int mm_xid_match_fn(const void *key1, const void *key2, Size keysize)
{
	return *(TransactionId*)key1 - *(TransactionId*)key2;
}

static void MMInitialize()
{
	bool found;
	static HASHCTL info;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	MM = ShmemInitStruct("MM", sizeof(MMState), &found);
	if (!found)
	{
		MM->hashLock = LWLockAssign();
        MM->nNodes = MMNodes;
	}
	LWLockRelease(AddinShmemInitLock);

	info.keysize = sizeof(TransactionId);
	info.entrysize = sizeof(LocalTransaction);
	info.hash = mm_xid_hash_fn;
	info.match = mm_xid_match_fn;
	MMLocalTrans = ShmemInitHash(
		"local_trans",
		MM_HASH_SIZE, MM_HASH_SIZE,
		&info,
		HASH_ELEM | HASH_FUNCTION | HASH_COMPARE
	);
}

/*
 *  ***************************************************************************
 */

void
_PG_init(void)
{
	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out without hooking into any of
	 * the main system.  (We don't throw error here because it seems useful to
	 * allow the cs_* functions to be created even when the
	 * module isn't active.  The functions must protect themselves against
	 * being called then, however.)
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in imcs_shmem_startup().
	 */
	RequestAddinShmemSpace(MM_SHMEM_SIZE);
	RequestAddinLWLocks(1);

	DefineCustomStringVariable(
		"multimaster.conn_strings",
		"Multimaster node connection strings separated by commas, i.e. 'replication=database dbname=postgres host=localhost port=5001,replication=database dbname=postgres host=localhost port=5002'",
		NULL,
		&MMConnStrs,
		"",
		PGC_BACKEND, // context
		0, // flags,
		NULL, // GucStringCheckHook check_hook,
		NULL, // GucStringAssignHook assign_hook,
		NULL // GucShowHook show_hook
	);
    
	DefineCustomIntVariable(
		"multimaster.node_id",
		"Multimaster node ID",
		NULL,
		&MMNodeId,
		1,
		1,
		INT_MAX,
		PGC_BACKEND,
		0,
		NULL,
		NULL,
		NULL
	);
    
    MMNodes = MMStartReceivers(MMConnStrs, MMNodeId);
    if (MMNodes < 2) { 
        elog(ERROR, "Multimaster should have at least two nodes");
    }
	/*
	 * Install hooks.
	 */
	MMPrevShmemStartupHook = shmem_startup_hook;
	shmem_startup_hook = MMShmemStartup;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	shmem_startup_hook = MMPrevShmemStartupHook;
}


static void MMShmemStartup(void)
{
	if (MMPrevShmemStartupHook)
		MMPrevShmemStartupHook();
    MMInitialize();
}

/*
 *  ***************************************************************************
 */

void MMMarkTransAsLocal(TransactionId xid)
{
    LocalTransaction* lt;

    Assert(TransactionIdIsValid(xid));
    LWLockAcquire(MM->hashLock, LW_EXCLUSIVE);
    lt = hash_search(MMLocalTrans, &xid, HASH_ENTER, NULL);
    lt->count = MM->nNodes-1;
    LWLockRelease(MM->hashLock);
}

bool MMIsLocalTransaction(TransactionId xid)
{
    LocalTransaction* lt;
    bool result = false;
    LWLockAcquire(MM->hashLock, LW_EXCLUSIVE);
    lt = hash_search(MMLocalTrans, &xid, HASH_FIND, NULL);
    if (lt != NULL) { 
        result = true;
        Assert(lt->count > 0);
        if (--lt->count == 0) { 
            hash_search(MMLocalTrans, &xid, HASH_REMOVE, NULL);
        }
    }
    LWLockRelease(MM->hashLock);
    return result;
}

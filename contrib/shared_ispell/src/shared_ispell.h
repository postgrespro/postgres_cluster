#ifndef __SHARED_ISPELL_H__
#define __SHARED_ISPELL_H__

#include "storage/lwlock.h"
#include "utils/timestamp.h"
#include "tsearch/dicts/spell.h"
#include "tsearch/ts_public.h"

/* This segment is initialized in the first process that accesses it (see
 * ispell_shmem_startup function).
 */
#define SEGMENT_NAME "shared_ispell"

#define MAXLEN 255

typedef struct SharedIspellDict
{
	/* this is used for selecting the dictionary */
	char   *dictFile;
	char   *affixFile;
	int		nbytes;
	int		nwords;

	/* next dictionary in the chain (essentially a linked list) */
	struct SharedIspellDict *next;

	IspellDict dict;
} SharedIspellDict;

typedef struct SharedStopList
{
	char   *stopFile;
	int		nbytes;

	struct SharedStopList *next;

	StopList stop;
} SharedStopList;

/* used to allocate memory in the shared segment */
typedef struct SegmentInfo
{
	LWLockId	lock;
	char	   *firstfree;        /* first free address (always maxaligned) */
	size_t		available;        /* free space remaining at firstfree */
	Timestamp	lastReset;        /* last reset of the dictionary */

	/* the shared segment (info and data) */
	SharedIspellDict *shdict;
	SharedStopList	 *shstop;
} SegmentInfo;

/* used to keep track of dictionary in each backend */
typedef struct DictInfo
{
	Timestamp	lookup;

	char dictFile[MAXLEN];
	char affixFile[MAXLEN];
	char stopFile[MAXLEN];

	/* We split word list and affix list.
	 * In shdict we store a word list, word list will be stored in shared segment.
	 * In dict we store an affix list in each process.
	 */
	SharedIspellDict   *shdict;
	IspellDict			dict;
	SharedStopList	   *shstop;
} DictInfo;

#endif
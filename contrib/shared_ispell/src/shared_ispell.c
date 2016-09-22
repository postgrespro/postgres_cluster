/*
 * Shared ispell dictionary stored in a shared memory segment, so that
 * backends may save memory and CPU time. By default each connection
 * keeps a private copy of the dictionary, which is wasteful as the
 * dictionaries are copied in memory multiple times. The connections
 * also need to initialize the dictionary on their own, which may take
 * up to a few seconds.
 *
 * This means the connections are either long-lived (and each keeps
 * a private copy of the dictionary, wasting memory), or short-lived
 * (resulting in high latencies when the dictionary is initialized).
 *
 * This extension is storing a single copy of the dictionary in a shared
 * memory so that all connections may use it, saving memory and CPU time.
 *
 *
 * The flow within the shared ispell may be slightly confusing, so this
 * is a brief summary of the main flows within the code.
 *
 * ===== shared segment init (postmaster startup) =====
 *
 * _PG_init
 *      -> ispell_shmem_startup (registered as a hook)
 *
 * ===== dictionary init (backend) =====
 *
 * dispell_init
 *      -> init_shared_dict
 *          -> get_shared_dict
 *              -> NIStartBuild
 *              -> NIImportDictionary
 *              -> NIImportAffixes
 *              -> NISortDictionary
 *              -> NISortAffixes
 *              -> NIFinishBuild
 *              -> sizeIspellDict
 *              -> copyIspellDict
 *                  -> copySPNode
 *          -> get_shared_stop_list
 *              -> readstoplist
 *              -> copyStopList
 *
 * ===== dictionary reinit after reset (backend) =====
 *
 * dispell_lexize
 *      -> timestamp of lookup < last reset
 *          -> init_shared_dict
 *              (see dispell_init above)
 *      -> SharedNINormalizeWord
*/

#include "postgres.h"
#include "miscadmin.h"
#include "storage/ipc.h"

#include "commands/defrem.h"
#include "tsearch/ts_locale.h"
#include "access/htup_details.h"
#include "funcapi.h"
#include "utils/builtins.h"

#include "shared_ispell.h"
#include "tsearch/dicts/spell.h"

PG_MODULE_MAGIC;

void _PG_init(void);
void _PG_fini(void);

/* Memory for dictionaries in kbytes */
static int max_ispell_mem_size_kb;

/* Saved hook values in case of unload */
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

/* These are used to allocate data within shared segment */
static SegmentInfo *segment_info = NULL;

static void ispell_shmem_startup(void);

static char *shalloc(int bytes);
static char *shstrcpy(char *str);

static SharedIspellDict *copyIspellDict(IspellDict *dict, char *dictFile, char *affixFile, int bytes, int words);
static SharedStopList *copyStopList(StopList *list, char *stopFile, int bytes);

static int sizeIspellDict(IspellDict *dict, char *dictFile, char *affixFile);
static int sizeStopList(StopList *list, char *stopFile);

/*
 * Get memory for dictionaries in bytes
 */
static Size
max_ispell_mem_size()
{
	return (Size)max_ispell_mem_size_kb * 1024L;
}

/*
 * Module load callback
 */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress) {
		elog(ERROR, "shared_ispell has to be loaded using shared_preload_libraries");
		return;
	}

	/* Define custom GUC variables. */

	/* How much memory should we preallocate for the dictionaries (limits how many
	 * dictionaries you can load into the shared segment). */
	DefineCustomIntVariable("shared_ispell.max_size",
							"amount of memory to pre-allocate for ispell dictionaries",
							NULL,
							&max_ispell_mem_size_kb,
							50 * 1024,			/* default 50MB */
							1024,				/* min 1MB */
							INT_MAX,
							PGC_POSTMASTER,
							GUC_UNIT_KB,
							NULL,
							NULL,
							NULL);

	EmitWarningsOnPlaceholders("shared_ispell");

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in ispell_shmem_startup().
	 */
	RequestAddinShmemSpace(max_ispell_mem_size());

	#if PG_VERSION_NUM >= 90600
	RequestNamedLWLockTranche("shared_ispell", 1);
	#else
	RequestAddinLWLocks(1);
	#endif

	/* Install hooks. */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = ispell_shmem_startup;
}


/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	shmem_startup_hook = prev_shmem_startup_hook;
}

/*
 * Probably the most important part of the startup - initializes the
 * memory in shared memory segment (creates and initializes the
 * SegmentInfo data structure).
 *
 * This is called from a shmem_startup_hook (see _PG_init).
 */
static void
ispell_shmem_startup()
{
	bool	found = FALSE;
	char   *segment;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	/*
	 * Create or attach to the shared memory state, including hash table
	 */
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	segment = ShmemInitStruct(SEGMENT_NAME,
							max_ispell_mem_size(),
							&found);
	segment_info = (SegmentInfo *) segment;

	/* Was the shared memory segment already initialized? */
	if (!found)
	{
		memset(segment, 0, max_ispell_mem_size());

		#if PG_VERSION_NUM >= 90600
		segment_info->lock = &(GetNamedLWLockTranche("shared_ispell"))->lock;
		#else
		segment_info->lock  = LWLockAssign();
		#endif
		segment_info->firstfree = segment + MAXALIGN(sizeof(SegmentInfo));
		segment_info->available = max_ispell_mem_size()
			- (int)(segment_info->firstfree - segment);

		segment_info->lastReset = GetCurrentTimestamp();
	}

	LWLockRelease(AddinShmemInitLock);
}

/*
 * This is called from backends that are looking up for a shared dictionary
 * definition using a filename with dictionary / affixes.
 *
 * This is called through dispell_init() which is responsible for proper locking
 * of the shared memory (using SegmentInfo->lock).
 */
static SharedIspellDict *
get_shared_dict(char *words, char *affixes)
{
	SharedIspellDict *dict = segment_info->shdict;

	while (dict != NULL)
	{
		if ((strcmp(dict->dictFile, words) == 0) &&
			(strcmp(dict->affixFile, affixes) == 0))
			return dict;
		dict = dict->next;
	}

	return NULL;
}

/*
 * This is called from backends that are looking up for a list of stop words
 * using a filename of the list.
 *
 * This is called through dispell_init() which is responsible for proper locking
 * of the shared memory (using SegmentInfo->lock).
 */
static SharedStopList *
get_shared_stop_list(char *stop)
{
	SharedStopList *list = segment_info->shstop;

	while (list != NULL)
	{
		if (strcmp(list->stopFile, stop) == 0)
			return list;
		list = list->next;
	}

	return NULL;
}

/*
 * Cleares IspellDict fields which are used for store affix list.
 */
static void
clean_dict_affix(IspellDict *dict)
{
	dict->maffixes = 0;
	dict->naffixes = 0;
	dict->Affix = NULL;

	dict->Suffix = NULL;
	dict->Prefix = NULL;

	dict->AffixData = NULL;
	dict->lenAffixData = 0;
	dict->nAffixData = 0;

	dict->CompoundAffix = NULL;
	dict->CompoundAffixFlags = NULL;
	dict->nCompoundAffixFlag = 0;
	dict->mCompoundAffixFlag = 0;

	dict->avail = 0;
}

/*
 * Initializes the dictionary for use in backends - checks whether such dictionary
 * and list of stopwords is already used, and if not then parses it and loads it into
 * the shared segment.
 *
 * Function lookup if the dictionary (word list) is already loaded in the
 * shared segment. If not then loads the dictionary (word list).
 * Affix list is loaded to a current backend process.
 *
 * This is called through dispell_init() which is responsible for proper locking
 * of the shared memory (using SegmentInfo->lock).
 */
static void
init_shared_dict(DictInfo *info, char *dictFile, char *affFile, char *stopFile)
{
	int size;

	SharedIspellDict *shdict = NULL;
	SharedStopList	 *shstop = NULL;

	IspellDict *dict;
	StopList	stoplist;

	/* DICTIONARY + AFFIXES */

	/* TODO This should probably check that the filenames are not NULL, and maybe that
	 * it exists. Or maybe that's handled by the NIImport* functions. */

	/* lookup if the dictionary (words and affixes) is already loaded in the shared segment */
	shdict = get_shared_dict(dictFile, affFile);

	/* clear dict affix sources */
	clean_dict_affix(&(info->dict));

	/* load affix list */
	NIStartBuild(&(info->dict));
	NIImportAffixes(&(info->dict), get_tsearch_config_filename(affFile, "affix"));

	/* load the dictionary (word list) if not yet defined */
	if (shdict == NULL)
	{
		dict = (IspellDict *) palloc0(sizeof(IspellDict));

		NIStartBuild(dict);
		NIImportDictionary(dict, get_tsearch_config_filename(dictFile, "dict"));

		dict->usecompound = info->dict.usecompound;

		dict->nCompoundAffixFlag = dict->mCompoundAffixFlag =
			info->dict.nCompoundAffixFlag;
		dict->CompoundAffixFlags = (CompoundAffixFlag *) palloc0(
			dict->nCompoundAffixFlag * sizeof(CompoundAffixFlag));
		memcpy(dict->CompoundAffixFlags, info->dict.CompoundAffixFlags,
			   dict->nCompoundAffixFlag * sizeof(CompoundAffixFlag));

		/*
		 * If affix->useFlagAliases == true then AffixData is generated
		 * in NIImportAffixes(). Therefore we need to copy it.
		 */
		if (info->dict.useFlagAliases)
		{
			int i;
			dict->useFlagAliases = true;
			dict->lenAffixData = info->dict.lenAffixData;
			dict->nAffixData = info->dict.nAffixData;
			dict->AffixData = (char **) palloc0(dict->nAffixData * sizeof(char *));
			for (i = 0; i < dict->nAffixData; i++)
			{
				dict->AffixData[i] = palloc0(strlen(info->dict.AffixData[i]) + 1);
				strcpy(dict->AffixData[i], info->dict.AffixData[i]);
			}
		}

		NISortDictionary(dict);
		NIFinishBuild(dict);

		/* check available space in shared segment */
		size = sizeIspellDict(dict, dictFile, affFile);
		if (size > segment_info->available)
			elog(ERROR, "shared dictionary %s.dict / %s.affix needs %d B, only %zd B available",
				dictFile, affFile, size, segment_info->available);

		/* fine, there's enough space - copy the dictionary */
		shdict = copyIspellDict(dict, dictFile, affFile, size, dict->nspell);
		shdict->dict.naffixes = info->dict.naffixes;

		/* add the new dictionary to the linked list (of SharedIspellDict structures) */
		shdict->next = segment_info->shdict;
		segment_info->shdict = shdict;
	}
	/* continue load affix list to a current backend process */

	/* NISortAffixes is used AffixData. Therefore we need to copy pointer */
	info->dict.lenAffixData = shdict->dict.lenAffixData;
	info->dict.nAffixData = shdict->dict.nAffixData;
	info->dict.AffixData = shdict->dict.AffixData;
	info->dict.Dictionary = shdict->dict.Dictionary;
	NISortAffixes(&(info->dict));
	NIFinishBuild(&(info->dict));

	/* STOP WORDS */

	/* lookup if the stop words are already loaded in the shared segment, but only if there
	 * actually is a list */
	if (stopFile && *stopFile)
	{
		shstop = get_shared_stop_list(stopFile);

		/* load the stopwords if not yet defined */
		if (shstop == NULL)
		{
			readstoplist(stopFile, &stoplist, lowerstr);

			size = sizeStopList(&stoplist, stopFile);
			if (size > segment_info->available)
				elog(ERROR, "shared stoplist %s.stop needs %d B, only %zd B available",
					stopFile, size, segment_info->available);

			/* fine, there's enough space - copy the stoplist */
			shstop = copyStopList(&stoplist, stopFile, size);

			/* add the new stopword list to the linked list (of SharedStopList structures) */
			shstop->next = segment_info->shstop;
			segment_info->shstop = shstop;
		}
	}

	/* Now, fill the DictInfo structure for the backend (references to dictionary,
	 * stopwords and the filenames). */

	info->shdict = shdict;
	info->shstop = shstop;
	info->lookup = GetCurrentTimestamp();

	memcpy(info->dictFile, dictFile, strlen(dictFile) + 1);
	memcpy(info->affixFile, dictFile, strlen(affFile)+ 1);
	if (stopFile != NULL)
		memcpy(info->stopFile, dictFile, strlen(stopFile) + 1);
	else
		memset(info->stopFile, 0, sizeof(info->stopFile));
}

Datum dispell_init(PG_FUNCTION_ARGS);
Datum dispell_lexize(PG_FUNCTION_ARGS);
Datum dispell_reset(PG_FUNCTION_ARGS);
Datum dispell_mem_available(PG_FUNCTION_ARGS);
Datum dispell_mem_used(PG_FUNCTION_ARGS);
Datum dispell_list_dicts(PG_FUNCTION_ARGS);
Datum dispell_list_stoplists(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(dispell_init);
PG_FUNCTION_INFO_V1(dispell_lexize);
PG_FUNCTION_INFO_V1(dispell_reset);
PG_FUNCTION_INFO_V1(dispell_mem_available);
PG_FUNCTION_INFO_V1(dispell_mem_used);
PG_FUNCTION_INFO_V1(dispell_list_dicts);
PG_FUNCTION_INFO_V1(dispell_list_stoplists);

/*
 * Resets the shared dictionary memory, i.e. removes all the dictionaries. This
 * is the only way to remove dictionaries from the memory - either when
 * a dictionary is no longer needed or needs to be reloaded (e.g. to update
 * list of words / affixes).
 */
Datum
dispell_reset(PG_FUNCTION_ARGS)
{
	LWLockAcquire(segment_info->lock, LW_EXCLUSIVE);

	segment_info->shdict = NULL;
	segment_info->shstop = NULL;
	segment_info->lastReset = GetCurrentTimestamp();
	segment_info->firstfree = ((char*) segment_info) + MAXALIGN(sizeof(SegmentInfo));
	segment_info->available = max_ispell_mem_size() - (int)(segment_info->firstfree - (char*) segment_info);

	memset(segment_info->firstfree, 0, segment_info->available);

	LWLockRelease(segment_info->lock);

	PG_RETURN_VOID();
}

/*
 * Returns amount of 'free space' in the shared segment (usable for dictionaries).
 */
Datum
dispell_mem_available(PG_FUNCTION_ARGS)
{
	int result = 0;
	LWLockAcquire(segment_info->lock, LW_SHARED);

	result = segment_info->available;

	LWLockRelease(segment_info->lock);

	PG_RETURN_INT32(result);
}

/*
 * Returns amount of 'occupied space' in the shared segment (used by current dictionaries).
 */
Datum
dispell_mem_used(PG_FUNCTION_ARGS)
{
	int result = 0;
	LWLockAcquire(segment_info->lock, LW_SHARED);

	result = max_ispell_mem_size() - segment_info->available;

	LWLockRelease(segment_info->lock);

	PG_RETURN_INT32(result);
}

/*
 * This initializes a (shared) dictionary for a backend. The function receives
 * a list of options specified in the CREATE TEXT SEARCH DICTIONARY with ispell
 * template (http://www.postgresql.org/docs/9.3/static/sql-createtsdictionary.html).
 *
 * There are three allowed options: DictFile, AffFile, StopWords. The values
 * should match to filenames in `pg_config --sharedir` directory, ending with
 * .dict, .affix and .stop.
 *
 * The StopWords parameter is optional, the two other are required.
 *
 * If any of the filenames are incorrect, the call to init_shared_dict will fail.
 */
Datum
dispell_init(PG_FUNCTION_ARGS)
{
	List	   *dictoptions = (List *) PG_GETARG_POINTER(0);
	char	   *dictFile = NULL,
			   *affFile = NULL,
			   *stopFile = NULL;
	bool		affloaded = false,
				dictloaded = false,
				stoploaded = false;
	ListCell   *l;

	/* this is the result passed to dispell_lexize */
	DictInfo   *info = (DictInfo *) palloc0(sizeof(DictInfo));

	foreach(l, dictoptions)
	{
		DefElem *defel = (DefElem *) lfirst(l);

		if (pg_strcasecmp(defel->defname, "DictFile") == 0)
		{
			if (dictloaded)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("multiple DictFile parameters")));
			dictFile = defGetString(defel);
			dictloaded = true;
		}
		else if (pg_strcasecmp(defel->defname, "AffFile") == 0)
		{
			if (affloaded)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("multiple AffFile parameters")));
			affFile = defGetString(defel);
			affloaded = true;
		}
		else if (pg_strcasecmp(defel->defname, "StopWords") == 0)
		{
			if (stoploaded)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("multiple StopWords parameters")));
			stopFile = defGetString(defel);
			stoploaded = true;
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("unrecognized Ispell parameter: \"%s\"",
							defel->defname)));
		}
	}

	if (!affloaded)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("missing AffFile parameter")));
	}
	else if (!dictloaded)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("missing DictFile parameter")));
	}

	/* search if the dictionary is already initialized */
	LWLockAcquire(segment_info->lock, LW_EXCLUSIVE);

	init_shared_dict(info, dictFile, affFile, stopFile);

	LWLockRelease(segment_info->lock);

	PG_RETURN_POINTER(info);
}

Datum
dispell_lexize(PG_FUNCTION_ARGS)
{
	DictInfo   *info = (DictInfo *) PG_GETARG_POINTER(0);
	char	   *in = (char *) PG_GETARG_POINTER(1);
	int32		len = PG_GETARG_INT32(2);
	char	   *txt;
	TSLexeme   *res;
	TSLexeme   *ptr,
	           *cptr;

	if (len <= 0)
		PG_RETURN_POINTER(NULL);

	txt = lowerstr_with_len(in, len);

	/* need to lock the segment in shared mode */
	LWLockAcquire(segment_info->lock, LW_SHARED);

	/* do we need to reinit the dictionary? was the dict reset since the lookup */
	if (timestamp_cmp_internal(info->lookup, segment_info->lastReset) < 0)
	{
		/* relock in exclusive mode */
		LWLockRelease(segment_info->lock);
		LWLockAcquire(segment_info->lock, LW_EXCLUSIVE);

		init_shared_dict(info, info->dictFile, info->affixFile, info->stopFile);
	}

	res = NINormalizeWord(&(info->dict), txt);

	/* nothing found :-( */
	if (res == NULL)
	{
		LWLockRelease(segment_info->lock);
		PG_RETURN_POINTER(NULL);
	}

	ptr = cptr = res;
	while (ptr->lexeme)
	{
		if (info->shstop && searchstoplist(&(info->shstop->stop), ptr->lexeme))
		{
			pfree(ptr->lexeme);
			ptr->lexeme = NULL;
			ptr++;
		}
		else
		{
			memcpy(cptr, ptr, sizeof(TSLexeme));
			cptr++;
			ptr++;
		}
	}
	cptr->lexeme = NULL;

	LWLockRelease(segment_info->lock);

	PG_RETURN_POINTER(res);
}

/*
 * This 'allocates' memory in the shared segment - i.e. the memory is
 * already allocated and this just gives nbytes to the caller. This is
 * used exclusively by the 'copy' methods defined below.
 *
 * The memory is kept aligned thanks to MAXALIGN. Also, this assumes
 * the segment was locked properly by the caller.
 */
static char *
shalloc(int bytes)
{
	char *result;
	bytes = MAXALIGN(bytes);

	/* This shouldn't really happen, as the init_shared_dict checks the size
	 * prior to copy. So let's just throw error here, as something went
	 * obviously wrong. */
	if (bytes > segment_info->available)
		elog(ERROR, "the shared segment (shared ispell) is too small");

	result = segment_info->firstfree;
	segment_info->firstfree += bytes;
	segment_info->available -= bytes;

	memset(result, 0, bytes);

	return result;
}

/*
 * Copies a string into the shared segment - allocates memory and does memcpy.
 *
 * TODO This assumes the string is properly terminated (should be guaranteed
 * by the code that reads and parses the dictionary / affixes).
 */
static char *
shstrcpy(char *str)
{
	char *tmp = shalloc(strlen(str) + 1);
	memcpy(tmp, str, strlen(str) + 1);
	return tmp;
}

/*
 * The following methods serve to do a "deep copy" of the parsed dictionary,
 * into the shared memory segment. For each structure this provides 'size'
 * and 'copy' functions to get the size first (for shalloc) and performing
 * the actual copy.
 */

/* SPNode - dictionary words */

static SPNode *
copySPNode(SPNode *node)
{
	int		i;
	SPNode *copy = NULL;

	if (node == NULL)
	    return NULL;

	copy = (SPNode *) shalloc(offsetof(SPNode, data) + sizeof(SPNodeData) * node->length);
	memcpy(copy, node, offsetof(SPNode, data) + sizeof(SPNodeData) * node->length);

	for (i = 0; i < node->length; i++)
	    copy->data[i].node = copySPNode(node->data[i].node);

	return copy;
}

static int
sizeSPNode(SPNode *node)
{
	int i;
	int size = 0;

	if (node == NULL)
	    return 0;

	size = MAXALIGN(offsetof(SPNode, data) + sizeof(SPNodeData) * node->length);

	for (i = 0; i < node->length; i++)
		size += sizeSPNode(node->data[i].node);

	return size;
}

/* StopList */

static SharedStopList *
copyStopList(StopList *list, char *stopFile, int size)
{
	int				i;
	SharedStopList *copy = (SharedStopList *) shalloc(sizeof(SharedStopList));

	copy->stop.len = list->len;
	copy->stop.stop = (char **) shalloc(sizeof(char *) * list->len);
	copy->stopFile = shstrcpy(stopFile);
	copy->nbytes = size;

	for (i = 0; i < list->len; i++)
		copy->stop.stop[i] = shstrcpy(list->stop[i]);

	return copy;
}

static int
sizeStopList(StopList *list, char *stopFile)
{
	int i;
	int size = MAXALIGN(sizeof(SharedStopList));

	size += MAXALIGN(sizeof(char *) * list->len);
	size += MAXALIGN(strlen(stopFile) + 1);

	for (i = 0; i < list->len; i++)
		size += MAXALIGN(strlen(list->stop[i]) + 1);

	return size;
}

/*
 * Performs deep copy of the dictionary into the shared memory segment.
 *
 * It gets the populated Ispell Dictionary (dict) and copies all the data
 * using the 'copy' methods listed above. It also keeps the filenames so
 * that it's possible to lookup the dictionaries later.
 *
 * Function copies only word list. Affix list is loaded to a current process.
 */
static SharedIspellDict *
copyIspellDict(IspellDict *dict, char *dictFile, char *affixFile, int size, int words)
{
	int i;

	SharedIspellDict *copy = (SharedIspellDict *) shalloc(sizeof(SharedIspellDict));

	copy->dictFile = shalloc(strlen(dictFile) + 1);
	copy->affixFile = shalloc(strlen(affixFile) + 1);

	strcpy(copy->dictFile, dictFile);
	strcpy(copy->affixFile, affixFile);

	copy->dict.Dictionary = copySPNode(dict->Dictionary);

	/* copy affix data */
	copy->dict.nAffixData = dict->nAffixData;
	copy->dict.AffixData = (char **) shalloc(sizeof(char *) * dict->nAffixData);
	for (i = 0; i < copy->dict.nAffixData; i++)
		copy->dict.AffixData[i] = shstrcpy(dict->AffixData[i]);

	copy->nbytes = size;
	copy->nwords = words;

	return copy;
}

/*
 * Computes how much space is needed for a dictionary (word list) in the shared segment.
 *
 * Function does not compute space for a affix list since affix list is loaded
 * to a current process.
 */
static int
sizeIspellDict(IspellDict *dict, char *dictFile, char *affixFile)
{
	int i;
	int size = MAXALIGN(sizeof(SharedIspellDict));

	size += MAXALIGN(strlen(dictFile) + 1);
	size += MAXALIGN(strlen(affixFile) + 1);

	size += sizeSPNode(dict->Dictionary);

	/* copy affix data */
	size += MAXALIGN(sizeof(char *) * dict->nAffixData);
	for (i = 0; i < dict->nAffixData; i++)
	    size += MAXALIGN(sizeof(char) * strlen(dict->AffixData[i]) + 1);

	return size;
}

/* SRF function returning a list of shared dictionaries currently loaded in memory. */
Datum
dispell_list_dicts(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	TupleDesc			tupdesc;
	SharedIspellDict   *dict;

	/* init on the first call */
	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* get a shared lock and then the first dictionary */
		LWLockAcquire(segment_info->lock, LW_SHARED);
		funcctx->user_fctx = segment_info->shdict;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		    ereport(ERROR,
		            (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		             errmsg("function returning record called in context "
		                    "that cannot accept type record")));

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->tuple_desc = tupdesc;

		/* switch back to the old context */
		MemoryContextSwitchTo(oldcontext);
	}

	/* init the context */
	funcctx = SRF_PERCALL_SETUP();

	/* check if we have more data */
	if (funcctx->user_fctx != NULL)
	{
		HeapTuple	tuple;
		Datum		result;
		Datum		values[5];
		bool		nulls[5];

		text	   *dictname,
				   *affname;

		dict = (SharedIspellDict *) funcctx->user_fctx;
		funcctx->user_fctx = dict->next;

		memset(nulls, 0, sizeof(nulls));

		dictname = cstring_to_text(dict->dictFile);
		affname  = cstring_to_text(dict->affixFile);

		values[0] = PointerGetDatum(dictname);
		values[1] = PointerGetDatum(affname);
		values[2] = UInt32GetDatum(dict->nwords);
		values[3] = UInt32GetDatum(dict->dict.naffixes);
		values[4] = UInt32GetDatum(dict->nbytes);

		/* Build and return the tuple. */
		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* Here we want to return another item: */
		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		/* release the lock */
		LWLockRelease(segment_info->lock);

		/* Here we are done returning items and just need to clean up: */
		SRF_RETURN_DONE(funcctx);
	}
}

/* SRF function returning a list of shared stopword lists currently loaded in memory. */
Datum
dispell_list_stoplists(PG_FUNCTION_ARGS)
{
	FuncCallContext	   *funcctx;
	TupleDesc			tupdesc;
	SharedStopList	   *stoplist;

	/* init on the first call */
	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* get a shared lock and then the first stop list */
		LWLockAcquire(segment_info->lock, LW_SHARED);
		funcctx->user_fctx = segment_info->shstop;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("function returning record called in context "
							"that cannot accept type record")));

		/*
		 * generate attribute metadata needed later to produce tuples from raw
		 * C strings
		 */
		funcctx->attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->tuple_desc = tupdesc;

		/* switch back to the old context */
		MemoryContextSwitchTo(oldcontext);
	}

	/* init the context */
	funcctx = SRF_PERCALL_SETUP();

	/* check if we have more data */
	if (funcctx->user_fctx != NULL)
	{
		HeapTuple	tuple;
		Datum		result;
		Datum		values[3];
		bool		nulls[3];

		text	   *stopname;

		stoplist = (SharedStopList *) funcctx->user_fctx;
		funcctx->user_fctx = stoplist->next;

		memset(nulls, 0, sizeof(nulls));

		stopname = cstring_to_text(stoplist->stopFile);

		values[0] = PointerGetDatum(stopname);
		values[1] = UInt32GetDatum(stoplist->stop.len);
		values[2] = UInt32GetDatum(stoplist->nbytes);

		/* Build and return the tuple. */
		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		/* make the tuple into a datum */
		result = HeapTupleGetDatum(tuple);

		/* Here we want to return another item: */
		SRF_RETURN_NEXT(funcctx, result);
	}
	else
	{
		/* release the lock */
		LWLockRelease(segment_info->lock);

		/* Here we are done returning items and just need to clean up: */
		SRF_RETURN_DONE(funcctx);
	}
}

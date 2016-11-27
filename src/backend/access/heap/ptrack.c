#include "postgres.h"

#include "access/heapam_xlog.h"
#include "access/heapam.h"
#include "access/ptrack.h"
#include "access/xlog.h"
#include "access/xlogutils.h"
#include "access/skey.h"
#include "access/genam.h"
#include "catalog/pg_depend.h"
#include "access/htup_details.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "storage/smgr.h"
#include "utils/inval.h"
#include "utils/array.h"
#include "utils/relfilenodemap.h"
#include <unistd.h>
#include <sys/stat.h>
/* Effective data size */
#define MAPSIZE (BLCKSZ - MAXALIGN(SizeOfPageHeaderData))

/* Number of bits allocated for each heap block. */
#define BITS_PER_HEAPBLOCK 1

/* Number of heap blocks we can represent in one byte. */
#define HEAPBLOCKS_PER_BYTE 8

#define HEAPBLK_TO_MAPBLOCK(x) ((x) / HEAPBLOCKS_PER_PAGE)
#define HEAPBLK_TO_MAPBYTE(x) (((x) % HEAPBLOCKS_PER_PAGE) / HEAPBLOCKS_PER_BYTE)
#define HEAPBLK_TO_MAPBIT(x) ((x) % HEAPBLOCKS_PER_BYTE)

#define HEAPBLOCKS_PER_PAGE (MAPSIZE * HEAPBLOCKS_PER_BYTE)

typedef struct BlockTrack
{
	BlockNumber		block_number;
	RelFileNode		rel;
} BlockTrack;

static BlockTrack blocks_track[XLR_MAX_BLOCK_ID];
unsigned int blocks_track_count = 0;
bool ptrack_enable = false;

static Buffer ptrack_readbuf(RelFileNode rnode, BlockNumber blkno, bool extend);
static void ptrack_extend(SMgrRelation smgr, BlockNumber nvmblocks);
static void ptrack_set(BlockNumber heapBlk, Buffer vmBuf);
void SetPtrackClearLSN(bool set_invalid);
Datum pg_ptrack_test(PG_FUNCTION_ARGS);

/* Tracking memory block inside critical zone */
void
ptrack_add_block(BlockNumber block_number, RelFileNode rel)
{
	BlockTrack *bt = &blocks_track[blocks_track_count];
	bt->block_number = block_number;
	bt->rel = rel;
	blocks_track_count++;
	Assert(blocks_track_count < XLR_MAX_BLOCK_ID);
}

/* Save tracked memory block after end of critical zone */
void
ptrack_save(void)
{
	Buffer pbuf = InvalidBuffer;
	unsigned int i;

	for (i = 0; i < blocks_track_count; i++)
	{
		BlockTrack *bt = &blocks_track[i];
		BlockNumber mapBlock = HEAPBLK_TO_MAPBLOCK(bt->block_number);

		/* Reuse the old pinned buffer if possible */
		if (BufferIsValid(pbuf))
		{
			if (BufferGetBlockNumber(pbuf) == mapBlock)
				goto set_bit;
			else
				ReleaseBuffer(pbuf);
		}

		pbuf = ptrack_readbuf(bt->rel, mapBlock, true);
		set_bit:
		ptrack_set(bt->block_number, pbuf);
	}
	if (pbuf != InvalidBuffer)
		ReleaseBuffer(pbuf);

	blocks_track_count = 0;
}

/* Set one bit to buffer */
void
ptrack_set(BlockNumber heapBlk, Buffer vmBuf)
{
	BlockNumber mapBlock = HEAPBLK_TO_MAPBLOCK(heapBlk);
	uint32		mapByte = HEAPBLK_TO_MAPBYTE(heapBlk);
	uint8		mapBit = HEAPBLK_TO_MAPBIT(heapBlk);
	Page		page;
	char	   *map;

	/* Check that we have the right VM page pinned */
	if (!BufferIsValid(vmBuf) || BufferGetBlockNumber(vmBuf) != mapBlock)
		elog(ERROR, "wrong VM buffer passed to ptrack_set");
	page = BufferGetPage(vmBuf);
	map = PageGetContents(page);
	LockBuffer(vmBuf, BUFFER_LOCK_SHARE);

	if (!(map[mapByte] & (1 << mapBit)))
	{
		/* Bad luck. Take an exclusive lock now after unlock share.*/
		LockBuffer(vmBuf, BUFFER_LOCK_UNLOCK);
		LockBuffer(vmBuf, BUFFER_LOCK_EXCLUSIVE);
		if (!(map[mapByte] & (1 << mapBit)))
		{
			START_CRIT_SECTION();

			map[mapByte] |= (1 << mapBit);
			MarkBufferDirty(vmBuf);

			END_CRIT_SECTION_WITHOUT_TRACK();
		}
	}

	LockBuffer(vmBuf, BUFFER_LOCK_UNLOCK);
}

static Buffer
ptrack_readbuf(RelFileNode rnode, BlockNumber blkno, bool extend)
{
	Buffer		buf;

	SMgrRelation smgr = smgropen(rnode, InvalidBackendId);

	/*
	 * If we haven't cached the size of the ptrack map fork yet, check it
	 * first.
	 */
	if (smgr->smgr_ptrack_nblocks == InvalidBlockNumber)
	{
		if (smgrexists(smgr, PAGESTRACK_FORKNUM))
			smgr->smgr_ptrack_nblocks = smgrnblocks(smgr,
													  PAGESTRACK_FORKNUM);
		else
			smgr->smgr_ptrack_nblocks = 0;
	}
	/* Handle requests beyond EOF */
	if (blkno >= smgr->smgr_ptrack_nblocks)
	{
		if (extend)
			ptrack_extend(smgr, blkno + 1);
		else
			return InvalidBuffer;
	}

	/*
	 * Use ZERO_ON_ERROR mode, and initialize the page if necessary. It's
	 * always safe to clear bits, so it's better to clear corrupt pages than
	 * error out.
	 */
	buf = ReadBufferWithoutRelcache2(smgr, PAGESTRACK_FORKNUM, blkno,
							 RBM_ZERO_ON_ERROR, NULL);

	if (PageIsNew(BufferGetPage(buf)))
	{
		Page pg = BufferGetPage(buf);
		PageInit(pg, BLCKSZ, 0);
	}
	return buf;
}

static void
ptrack_extend(SMgrRelation smgr, BlockNumber vm_nblocks)
{
	BlockNumber vm_nblocks_now;
	Page		pg;

	pg = (Page) palloc(BLCKSZ);
	PageInit(pg, BLCKSZ, 0);

	LockSmgrForExtension(smgr, ExclusiveLock);
	/*
	 * Create the file first if it doesn't exist.  If smgr_ptrack_nblocks is
	 * positive then it must exist, no need for an smgrexists call.
	 */
	if ((smgr->smgr_ptrack_nblocks == 0 ||
		 smgr->smgr_ptrack_nblocks == InvalidBlockNumber) &&
		!smgrexists(smgr, PAGESTRACK_FORKNUM))
		smgrcreate(smgr, PAGESTRACK_FORKNUM, false);

	vm_nblocks_now = smgrnblocks(smgr, PAGESTRACK_FORKNUM);

	/* Now extend the file */
	while (vm_nblocks_now < vm_nblocks)
	{
		PageSetChecksumInplace(pg, vm_nblocks_now);
		smgrextend(smgr, PAGESTRACK_FORKNUM, vm_nblocks_now,
				   (char *) pg, false);
		vm_nblocks_now++;
	}
	/*
	 * Send a shared-inval message to force other backends to close any smgr
	 * references they may have for this rel, which we are about to change.
	 * This is a useful optimization because it means that backends don't have
	 * to keep checking for creation or extension of the file, which happens
	 * infrequently.
	 */
	CacheInvalidateSmgr(smgr->smgr_rnode);
	/* Update local cache with the up-to-date size */
	smgr->smgr_ptrack_nblocks = vm_nblocks_now;

	pfree(pg);

	UnlockSmgrForExtension(smgr, ExclusiveLock);
}

/* Clear all ptrack files */
void
ptrack_clear(void)
{
	HeapTuple tuple;
	Relation catalog = heap_open(RelationRelationId, AccessShareLock);
	SysScanDesc scan = systable_beginscan(catalog, InvalidOid, false, NULL, 0, NULL);

	while (HeapTupleIsValid(tuple = systable_getnext(scan)))
	{
		BlockNumber nblock;
		Relation rel = RelationIdGetRelation(HeapTupleGetOid(tuple));

		RelationOpenSmgr(rel);
		if (rel->rd_smgr == NULL)
			goto end_rel;

		LockSmgrForExtension(rel->rd_smgr, ExclusiveLock);

		if (rel->rd_smgr->smgr_ptrack_nblocks == InvalidBlockNumber)
		{
			if (smgrexists(rel->rd_smgr, PAGESTRACK_FORKNUM))
				rel->rd_smgr->smgr_ptrack_nblocks = smgrnblocks(rel->rd_smgr,
														PAGESTRACK_FORKNUM);
			else
				rel->rd_smgr->smgr_ptrack_nblocks = 0;
		}

		for(nblock = 0; nblock < rel->rd_smgr->smgr_ptrack_nblocks; nblock++)
		{
			Buffer	buf = ReadBufferExtended(rel, PAGESTRACK_FORKNUM,
				   nblock, RBM_ZERO_ON_ERROR, NULL);
			Page page = BufferGetPage(buf);
			char *map = PageGetContents(page);
			LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
			START_CRIT_SECTION();
			MemSet(map, 0, MAPSIZE);
			MarkBufferDirty(buf);
			END_CRIT_SECTION_WITHOUT_TRACK();
			LockBuffer(buf, BUFFER_LOCK_UNLOCK);
			ReleaseBuffer(buf);
		}

		UnlockSmgrForExtension(rel->rd_smgr, ExclusiveLock);
		end_rel:
			RelationClose(rel);
	}

	systable_endscan(scan);
	heap_close(catalog, AccessShareLock);

	SetPtrackClearLSN(false);
}

/* Get ptrack file as bytea and clear him */
bytea *
ptrack_get_and_clear(Oid tablespace_oid, Oid table_oid)
{
	bytea *result = NULL;
	BlockNumber nblock;
        Relation rel = RelationIdGetRelation(RelidByRelfilenode(tablespace_oid, table_oid));

	if (table_oid == InvalidOid)
	{
		elog(WARNING, "InvalidOid");
		goto full_end;
	}

	if (rel == InvalidRelation)
	{
		elog(WARNING, "InvalidRelation");
		goto full_end;
	}

	RelationOpenSmgr(rel);
	if (rel->rd_smgr == NULL)
		goto end_rel;

	LockSmgrForExtension(rel->rd_smgr, ExclusiveLock);

	if (rel->rd_smgr->smgr_ptrack_nblocks == InvalidBlockNumber)
	{
		if (smgrexists(rel->rd_smgr, PAGESTRACK_FORKNUM))
			rel->rd_smgr->smgr_ptrack_nblocks = smgrnblocks(rel->rd_smgr,
															PAGESTRACK_FORKNUM);
		else
			rel->rd_smgr->smgr_ptrack_nblocks = 0;
	}
	if (rel->rd_smgr->smgr_ptrack_nblocks == 0)
	{
		UnlockSmgrForExtension(rel->rd_smgr, ExclusiveLock);
		goto end_rel;
	}
	result = (bytea *) palloc(rel->rd_smgr->smgr_ptrack_nblocks*MAPSIZE + VARHDRSZ);
	SET_VARSIZE(result, rel->rd_smgr->smgr_ptrack_nblocks*MAPSIZE + VARHDRSZ);

	for(nblock = 0; nblock < rel->rd_smgr->smgr_ptrack_nblocks; nblock++)
	{
		Buffer	buf = ReadBufferExtended(rel, PAGESTRACK_FORKNUM,
			   nblock, RBM_ZERO_ON_ERROR, NULL);
		Page page = BufferGetPage(buf);
		char *map = PageGetContents(page);
		LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
		START_CRIT_SECTION();
		memcpy(VARDATA(result) + nblock*MAPSIZE, map, MAPSIZE);
		MemSet(map, 0, MAPSIZE);
		MarkBufferDirty(buf);
		END_CRIT_SECTION_WITHOUT_TRACK();
		LockBuffer(buf, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buf);
	}

	UnlockSmgrForExtension(rel->rd_smgr, ExclusiveLock);
	end_rel:
		RelationClose(rel);

	SetPtrackClearLSN(false);
	full_end:
	if (result == NULL)
	{
		result = palloc0(VARHDRSZ);
		SET_VARSIZE(result, VARHDRSZ);
	}

	return result;
}

void
SetPtrackClearLSN(bool set_invalid)
{
	int			fd;
	XLogRecPtr	ptr;
	char		file_path[MAXPGPATH];
	if (set_invalid)
		ptr = InvalidXLogRecPtr;
	else
		ptr = GetXLogInsertRecPtr();

	join_path_components(file_path, DataDir, "global/ptrack_control");
	canonicalize_path(file_path);

	fd = BasicOpenFile(file_path,
					   O_RDWR | O_CREAT | PG_BINARY,
					   S_IRUSR | S_IWUSR);
	if (fd < 0)
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not create ptrack control file \"%s\": %m",
						"global/ptrack_control")));

	errno = 0;
	if (write(fd, &ptr, sizeof(XLogRecPtr)) != sizeof(XLogRecPtr))
	{
		/* if write didn't set errno, assume problem is no disk space */
		if (errno == 0)
			errno = ENOSPC;
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not write to ptrack control file: %m")));
	}

	if (pg_fsync(fd) != 0)
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not fsync ptrack control file: %m")));

	if (close(fd))
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not close ptrack control file: %m")));
}

/* Test ptrack file */
Datum
pg_ptrack_test(PG_FUNCTION_ARGS)
{
	Oid relation_oid = PG_GETARG_OID(0);
	BlockNumber nblock, num_blocks;
	Relation rel;
	XLogRecPtr ptrack_control_lsn;
	Buffer ptrack_buf = InvalidBuffer;
	Page		page;
	char	   *map;
	int			fd;
	unsigned int excess_data_counter = 0;
	unsigned int necessary_data_counter = 0;
	ArrayType *result_array;
	Datum result_elems[2];

	if (!superuser() && !has_rolreplication(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
		 (errmsg("must be superuser or replication role to clear ptrack files"))));

	/* get LSN from ptrack_control file */
	fd = BasicOpenFile("global/ptrack_control",
					   O_RDONLY | PG_BINARY,
					   0);

	if (fd < 0)
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not open ptrack control file \"%s\": %m",
						"global/ptrack_control")));
	errno = 0;
	if (read(fd, &ptrack_control_lsn, sizeof(XLogRecPtr)) != sizeof(XLogRecPtr))
	{
		/* if write didn't set errno, assume problem is no disk space */
		if (errno == 0)
			errno = ENOSPC;

		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not read to ptrack control file: %m")));
	}

	if (close(fd))
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not close ptrack control file: %m")));

	rel = RelationIdGetRelation(relation_oid);
	if (rel == InvalidRelation)
	{
		elog(WARNING, "Relation not found.");
		goto end_return;
	}
	LockRelationOid(relation_oid, AccessShareLock);
	RelationOpenSmgr(rel);
	if (rel->rd_smgr == NULL)
		goto end_rel;

	LockSmgrForExtension(rel->rd_smgr, ExclusiveLock);

	num_blocks = smgrnblocks(rel->rd_smgr, MAIN_FORKNUM);
	if (rel->rd_smgr->smgr_ptrack_nblocks == InvalidBlockNumber)
	{
		if (smgrexists(rel->rd_smgr, PAGESTRACK_FORKNUM))
			rel->rd_smgr->smgr_ptrack_nblocks = smgrnblocks(rel->rd_smgr, PAGESTRACK_FORKNUM);
		else
			rel->rd_smgr->smgr_ptrack_nblocks = 0;
	}

	for(nblock = 0; nblock < num_blocks; nblock++)
	{
		Buffer	main_buf = ReadBufferExtended(rel,
			MAIN_FORKNUM,
			nblock,
			RBM_ZERO_ON_ERROR,
			NULL);
		Page main_page;
		XLogRecPtr	main_page_lsn;

		BlockNumber mapBlock = HEAPBLK_TO_MAPBLOCK(nblock);
		uint32		mapByte = HEAPBLK_TO_MAPBYTE(nblock);
		uint8		mapBit = HEAPBLK_TO_MAPBIT(nblock);

		/* Get page lsn */
		LockBuffer(main_buf, BUFFER_LOCK_SHARE);
		main_page = BufferGetPage(main_buf);
		main_page_lsn = PageGetLSN(main_page);
		LockBuffer(main_buf, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(main_buf);

		/* Reuse the old pinned buffer if possible */
		if (BufferIsValid(ptrack_buf))
		{
			if (BufferGetBlockNumber(ptrack_buf) == mapBlock)
				goto read_bit;
			else
				ReleaseBuffer(ptrack_buf);
		}
		ptrack_buf = ptrack_readbuf(rel->rd_node, mapBlock, false);

		read_bit:
		if (ptrack_buf == InvalidBuffer)
		{
			/* not tracked data */
			if(ptrack_control_lsn < main_page_lsn)
			{
				necessary_data_counter++;
				elog(WARNING, "Block %ud not track. Ptrack lsn:%X/%X page lsn:%X/%X",
					nblock,
					(uint32) (ptrack_control_lsn >> 32),
					(uint32) ptrack_control_lsn,
					(uint32) (main_page_lsn >> 32),
					(uint32) main_page_lsn);
			}
			else
				continue;
		}

		page = BufferGetPage(ptrack_buf);
		map = PageGetContents(page);
		LockBuffer(ptrack_buf, BUFFER_LOCK_SHARE);
		if(map[mapByte] & (1 << mapBit))
		{
			/* excess data */
			if (ptrack_control_lsn >= main_page_lsn)
			{
				excess_data_counter++;
				elog(WARNING, "Block %ud not needed. Ptrack lsn:%X/%X page lsn:%X/%X",
					nblock,
					(uint32) (ptrack_control_lsn >> 32),
					(uint32) ptrack_control_lsn,
					(uint32) (main_page_lsn >> 32),
					(uint32) main_page_lsn);
			}
		}
		/* not tracked data */
		else if (ptrack_control_lsn < main_page_lsn)
		{
			necessary_data_counter++;
			elog(WARNING, "Block %ud not tracked. Ptrack lsn:%X/%X page lsn:%X/%X",
				nblock,
				 (uint32) (ptrack_control_lsn >> 32),
				 (uint32) ptrack_control_lsn,
				 (uint32) (main_page_lsn >> 32),
				 (uint32) main_page_lsn);
		}
		LockBuffer(ptrack_buf, BUFFER_LOCK_UNLOCK);
	}

	end_rel:
	if (ptrack_buf != InvalidBuffer)
		ReleaseBuffer(ptrack_buf);
	RelationClose(rel);
	UnlockRelationOid(relation_oid, AccessShareLock);

	end_return:
	result_elems[0] = UInt32GetDatum(excess_data_counter);
	result_elems[1] = UInt32GetDatum(necessary_data_counter);
	result_array = construct_array(result_elems, 2, 23, 4, true, 'i');
	PG_RETURN_ARRAYTYPE_P(result_array);
}

void
assign_ptrack_enable(bool newval, void *extra)
{
	if(DataDir != NULL && !IsBootstrapProcessingMode() && !newval)
		SetPtrackClearLSN(true);
}

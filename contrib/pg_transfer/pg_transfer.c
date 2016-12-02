/*-------------------------------------------------------------------------
 *
 * pg_transfer.c
 *	  transfer CONSTANT tables from one database to another.
 *
 *	  contrib/pg_transfer/pg_transfer.c
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/catalog.h"
#include "catalog/namespace.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/inval.h"
#include "access/generic_xlog.h"
#include "access/xlogutils.h"
#include "storage/smgr.h"
#include "access/visibilitymap.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_transfer_wal);
PG_FUNCTION_INFO_V1(pg_transfer_cleanup_shmem);
PG_FUNCTION_INFO_V1(pg_transfer_freeze);

static void
_update_toastrelid_in_varlena_attrs(Relation rel, HeapTuple oldtup, Oid newtoastoid);

/*
 * pg_transfer_wal()
 * Generate WAL for a given relation, its vm and fsm.
 * This function must be called, when table is restored on a master server,
 * in order to deliever all the files to the slave server using XLog streaming.
 */
Datum
pg_transfer_wal(PG_FUNCTION_ARGS)
{
	Oid			relOid = PG_GETARG_OID(0);
	Relation	rel;
	BlockNumber blkno;
	BlockNumber nblocks;

	rel = try_relation_open(relOid, AccessExclusiveLock);

	/* Sequence relations are not marked with flag constant */
	if (rel->rd_rel->relkind != RELKIND_SEQUENCE
		&& rel->rd_rel->relpersistence != RELPERSISTENCE_CONSTANT)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("cannot generate WAL for non-constant relation \"%s\"",
						RelationGetRelationName(rel))));

	nblocks = RelationGetNumberOfBlocks(rel);

	/* Create generic Xlog record for each block of the relation */
	for (blkno = 0; blkno < nblocks; blkno++)
	{
		Buffer	buffer;
		GenericXLogState *state;

		CHECK_FOR_INTERRUPTS();

		buffer = ReadBuffer(rel, blkno);
		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

		state = GenericXLogStart(rel);
		GenericXLogRegisterBuffer(state, buffer, GENERIC_XLOG_FULL_IMAGE);
		GenericXLogFinish(state);

		UnlockReleaseBuffer(buffer);
	}

	/* Generate WAL for visibility map, if any. */
	RelationOpenSmgr(rel);

	if (smgrexists(rel->rd_smgr, VISIBILITYMAP_FORKNUM))
		nblocks = RelationGetNumberOfBlocksInFork(rel, VISIBILITYMAP_FORKNUM);
	else
		nblocks = 0;

	for (blkno = 0; blkno < nblocks; blkno++)
	{
		Buffer		buffer;
		GenericXLogState *state;

		elog(DEBUG1, "DEBUG1. generateWAL for VM blkno %u", blkno);
		CHECK_FOR_INTERRUPTS();

		buffer = ReadBufferExtended(rel, VISIBILITYMAP_FORKNUM, blkno,
						RBM_ZERO_ON_ERROR, NULL);
		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

		state = GenericXLogStart(rel);
		GenericXLogRegisterBuffer(state, buffer, GENERIC_XLOG_FULL_IMAGE);
		GenericXLogFinish(state);

		UnlockReleaseBuffer(buffer);
	}

	/* Generate WAL for visibility map, if any. */
	if (smgrexists(rel->rd_smgr, FSM_FORKNUM))
		nblocks = RelationGetNumberOfBlocksInFork(rel, FSM_FORKNUM);
	else
		nblocks = 0;

	for (blkno = 0; blkno < nblocks; blkno++)
	{
		Buffer		buffer;
		GenericXLogState *state;

		elog(DEBUG1, "DEBUG1. generateWAL for FSM blkno %u", blkno);
		CHECK_FOR_INTERRUPTS();

		buffer = ReadBufferExtended(rel, FSM_FORKNUM, blkno,
						RBM_ZERO_ON_ERROR, NULL);
		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

		state = GenericXLogStart(rel);
		GenericXLogRegisterBuffer(state, buffer, GENERIC_XLOG_FULL_IMAGE);
		GenericXLogFinish(state);

		UnlockReleaseBuffer(buffer);
	}

	relation_close(rel, AccessExclusiveLock);
	PG_RETURN_VOID();
}

/*
 * pg_transfer_cleanup_shmem()
 * Unlink all the files of the relation and remove then from shared buffers.
 * This function must be called before we put new files into the directory.
 * When we restored schema at the first step of relation transfer, some files
 * could be created. We must sweep them out of buffer cache, otherwise upcoming
 * checkpoint can cause data loss flushing outdated pages on top of new ones.
 * NOTE. That is quite unoptimal, because DropRelFileNodesAllBuffers reads
 * BufferDescriptors sequentially.
 */
Datum
pg_transfer_cleanup_shmem(PG_FUNCTION_ARGS)
{
	Oid			relOid = PG_GETARG_OID(0);
	Relation	rel;
	SMgrRelation srel;
	RelFileNodeBackend rnode;

	rel = try_relation_open(relOid, AccessExclusiveLock);

	if (rel->rd_rel->relkind != RELKIND_SEQUENCE
		&& rel->rd_rel->relpersistence != RELPERSISTENCE_CONSTANT)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				errmsg("cannot invalidate buffers for non-constant relation \"%s\"",
						RelationGetRelationName(rel))));

	srel = smgropen(rel->rd_node, InvalidBackendId);
	rnode = srel->smgr_rnode;

	DropRelFileNodesAllBuffers(&rnode, 1);

	smgrclose(srel);

	relation_close(rel, AccessExclusiveLock);

	PG_RETURN_VOID();
}

/*
 * pg_transfer_freeze()
 * 		The function to prepare table for transfer.
 *
 * It does a number of important changes in one pass:
 * - sets FrozenXid for all alive tuples on the page.
 * - updates table's VM. Note, that if we met some dead tuples, we won't clean
 *   them up, just leave them in the table. But it means that we cannot set VM
 *   bits and Index-only scan would work worse than expected. Anyway, we rely
 *   on the user vacuumed table before transfer, so that won't be a problem.
 * - sets MinmalLSN for each page.
 * - updates toasttableoid stored in varlena attrs to the new value. If given
 *   newtoastoid is 0, skip this step. NOTE that after update of toasttableoids,
 *   it's dangerus to use the table, because it won't extract toast values
 *   correctly. Most probably it'll fail with an error "relation with OID ...
 *   is not found", but in the worst case it can return incorrect result.
 *	 So users have two options. They can drop the relation after dump,
 *   or they can update toasttableoid back to the initial value using the
 *   same function.
 */
Datum
pg_transfer_freeze(PG_FUNCTION_ARGS)
{
	Oid			relOid = PG_GETARG_OID(0);
	Oid			newtoastoid = PG_GETARG_OID(1);
	Relation	rel;
	BlockNumber blkno;
	BlockNumber nblocks;
	Buffer		vmbuffer = InvalidBuffer;
	BufferAccessStrategy vac_strategy = GetAccessStrategy(BAS_VACUUM);

	rel = try_relation_open(relOid, AccessExclusiveLock);

	if (rel->rd_rel->relpersistence != RELPERSISTENCE_CONSTANT)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("cannot freeze non-constant relation \"%s\"",
						RelationGetRelationName(rel))));
	if (rel->rd_rel->relkind != RELKIND_RELATION)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("cannot freeze non-table relation \"%s\"",
						RelationGetRelationName(rel))));

	nblocks = RelationGetNumberOfBlocks(rel);

	/*
	 * Examine all blocks of the given relation.
	 * In each block freeze all tuples which can be frozen.
	 * Freeze DEAD items, but do not remove them to avoid
	 * cleaning indexes.
	 */
	for (blkno = 0; blkno < nblocks; blkno++)
	{
		Buffer	buffer;
		Page	page;
		OffsetNumber offnum,
					maxoff;
		int ndead = 0;
		int nunused = 0;

		CHECK_FOR_INTERRUPTS();

		visibilitymap_pin(rel, blkno, &vmbuffer);

		buffer = ReadBufferExtended(rel, MAIN_FORKNUM, blkno,
								 RBM_NORMAL, vac_strategy);

		LockBufferForCleanup(buffer);
		page = BufferGetPage(buffer);

		if (PageIsNew(page))
		{
			elog(DEBUG1, "pg_transfer_freeze. page is new. blkno %u, rel %s",
							  blkno, RelationGetRelationName(rel));
			UnlockReleaseBuffer(buffer);
			continue;
		}

		if (PageIsEmpty(page))
		{
			elog(DEBUG1, "pg_transfer_freeze. page is empty. blkno %u, rel %s",
							  blkno, RelationGetRelationName(rel));
			UnlockReleaseBuffer(buffer);
			continue;
		}

		maxoff = PageGetMaxOffsetNumber(page);

		START_CRIT_SECTION();

		/*
		 * Iterate over all items on page and set frozen xid
		 */
		for (offnum = FirstOffsetNumber;
			 offnum <= maxoff;
			 offnum = OffsetNumberNext(offnum))
		{
			ItemId		itemid;
			HeapTupleData tuple;

			itemid = PageGetItemId(page, offnum);

			/* Unused items require no processing, but we count 'em */
			if (!ItemIdIsUsed(itemid))
			{
				nunused += 1;
				continue;
			}

			/* Redirect items need no processing too */
			if (ItemIdIsRedirected(itemid))
				continue;

			/*
			 * Don't freeze dead tuples. Since users don't expect to see them
			 * in restored table in new database.
			 */
			if (ItemIdIsDead(itemid))
			{
				ndead++;
				continue;
			}

			Assert(ItemIdIsNormal(itemid));

			ItemPointerSet(&(tuple.t_self), blkno, offnum);
			tuple.t_data = (HeapTupleHeader) PageGetItem(page, itemid);
			tuple.t_len = ItemIdGetLength(itemid);
			tuple.t_tableOid = RelationGetRelid(rel);

			HeapTupleSetXmin(&tuple, FrozenTransactionId);
			HeapTupleHeaderSetXminFrozen(tuple.t_data);
			HeapTupleHeaderSetCmin(tuple.t_data, FirstCommandId);
			HeapTupleSetXmax(&tuple, InvalidTransactionId);
			tuple.t_data->t_infomask |= HEAP_XMAX_INVALID;

			if (newtoastoid != InvalidOid)
				_update_toastrelid_in_varlena_attrs(rel, &tuple, newtoastoid);
		}

		if (ndead == 0)
			PageSetAllVisible(page);
		else
			elog(WARNING, "Block %u of table '%s' has %u dead tuples.\n"
						  "It will work unoptimal after transfer.\n"
						  "You should run VACUUM (ANALYZE) on the table and"
						  "then call pg_transfer_freeze() again.",
				 blkno, RelationGetRelationName(rel), ndead);

		/* Set the very minimal LSN for this page */
		PageSetLSN(page, SizeOfXLogLongPHD);

		MarkBufferDirty(buffer);
		END_CRIT_SECTION();

		/* Update visibilitymap bits */
		if (ndead == 0 && !VM_ALL_FROZEN(rel, blkno, &vmbuffer))
		{
			uint8 flags = VISIBILITYMAP_ALL_FROZEN | VISIBILITYMAP_ALL_VISIBLE;

			Assert(BufferIsValid(vmbuffer));

			elog(DEBUG1, "pg_transfer_freeze. visibilitymap_set FROZEN & VISIBLE. page %u",
							BufferGetBlockNumber(buffer));
			visibilitymap_set(rel, blkno, buffer, InvalidXLogRecPtr,
						vmbuffer, FrozenTransactionId, flags);
		}

		if (BufferIsValid(vmbuffer))
		{
			ReleaseBuffer(vmbuffer);
			vmbuffer = InvalidBuffer;
		}

		UnlockReleaseBuffer(buffer);
	}

	relation_close(rel, AccessExclusiveLock);
	PG_RETURN_VOID();
}

/*
 * _update_toastrelid_in_varlena_attrs()
 *		updates toasttableoid stored in varlena attrs to the new value.
 */
static void
_update_toastrelid_in_varlena_attrs(Relation rel, HeapTuple oldtup, Oid newtoastoid)
{
	HeapTupleHeader tup = oldtup->t_data;
	bits8	   *bp = tup->t_bits;	/* ptr to null bitmap in tuple */
	char	   *tp;				/* ptr to data part of tuple */
	TupleDesc	tupleDesc;
	Form_pg_attribute *att;
	int			numAttrs;
	int			attnum;
	bool		hasnulls = HeapTupleHasNulls(oldtup);
	long		off; /* offset in tuple data */

	/*
	 * We should only ever be called for tuples of plain relations.
	 */
	Assert(rel->rd_rel->relkind == RELKIND_RELATION);
	Assert(rel->rd_rel->relpersistence == RELPERSISTENCE_CONSTANT);

	/* Nothing to do. All fields are fixed. */
	if (HeapTupleAllFixed(oldtup))
		return;

	/*
	 * Get the tuple descriptor and break down the tuple into fields.
	 */
	tupleDesc = rel->rd_att;
	att = tupleDesc->attrs;
	numAttrs = tupleDesc->natts;
	tp = (char *) tup + tup->t_hoff;

	Assert(numAttrs <= MaxHeapAttributeNumber);

	off = 0;
	/*
	 * Check for external stored attributes and update their va_toastrelid
	 */
	for (attnum = 0; attnum < numAttrs; attnum++)
	{
		Form_pg_attribute thisatt = att[attnum];
		Datum attdatum;
		Pointer attptr;

		if (hasnulls && att_isnull(attnum, bp))
			continue;

		if (thisatt->attcacheoff >= 0)
			off = thisatt->attcacheoff;
		else if (thisatt->attlen != -1)
		{
			/* not varlena, so safe to use att_align_nominal */
			off = att_align_nominal(off, thisatt->attalign);
			thisatt->attcacheoff = off;
		}
		else
		{
			/* varlena attribute -> inspect it */
			if (off != att_align_nominal(off, thisatt->attalign))
			{
				off = att_align_pointer(off, thisatt->attalign, -1,
										tp + off);
			}

			attdatum = fetchatt(thisatt, tp + off);
			attptr = DatumGetPointer(attdatum);

			if (VARATT_IS_EXTERNAL_ONDISK(attptr))
			{
				/* Update attribute's va_toastrelid */
				struct varatt_external toast_pointer;
				varattrib_1b_e *attre = (varattrib_1b_e *) (attdatum);
				Pointer toast_pointer_ptr = (Pointer) attre->va_data;

				Assert(VARSIZE_EXTERNAL(attre) == sizeof(toast_pointer) + VARHDRSZ_EXTERNAL);
				/*
				 * since varlena attr can be stored unaligned,
				 * we must copy it to access structure fields
				 */
				memcpy(&(toast_pointer), toast_pointer_ptr, sizeof(toast_pointer));

				elog(DEBUG1, "_update_toastrelid_in_varlena_attrs. att %d off %ld va_toastrelid old %u, new %u",
					attnum, off, toast_pointer.va_toastrelid, newtoastoid);
				toast_pointer.va_toastrelid = newtoastoid;
				/* copy updated attr back to tuple */
				memcpy(toast_pointer_ptr, &(toast_pointer), sizeof(toast_pointer));
			}
		}

		off = att_addlength_pointer(off, thisatt->attlen, tp + off);
	}
}

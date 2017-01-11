#include <unistd.h>
#include "postgres.h"

#include "funcapi.h"
#include "libpq-fe.h"
#include "miscadmin.h"
#include "pgstat.h"

#include "access/htup_details.h"
#include "access/relscan.h"
#include "access/xact.h"
#include "access/clog.h"

#include "catalog/catversion.h"
#include "catalog/dependency.h"
#include "catalog/index.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"

#include "executor/spi.h"
#include "commands/vacuum.h"
#include "commands/defrem.h"
#include "parser/parse_utilcmd.h"

#include "libpq/pqformat.h"

#include "mb/pg_wchar.h"

#include "parser/parse_type.h"

#include "replication/logical.h"
#include "replication/origin.h"

#include "storage/ipc.h"
#include "storage/lmgr.h"
#include "storage/lwlock.h"
#include "storage/bufmgr.h"
#include "storage/proc.h"

#include "tcop/pquery.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"

#include "utils/array.h"
#include "utils/tqual.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "parser/parse_relation.h"

#include "multimaster.h"
#include "pglogical_relid_map.h"
#include "spill.h"

typedef struct TupleData
{
	Datum		values[MaxTupleAttributeNumber];
	bool		isnull[MaxTupleAttributeNumber];
	bool		changed[MaxTupleAttributeNumber];
} TupleData;

static Relation read_rel(StringInfo s, LOCKMODE mode);
static void read_tuple_parts(StringInfo s, Relation rel, TupleData *tup);
static EState* create_rel_estate(Relation rel);
static bool find_pkey_tuple(ScanKey skey, Relation rel, Relation idxrel,
                            TupleTableSlot *slot, bool lock, LockTupleMode mode);
static void build_index_scan_keys(EState *estate, ScanKey *scan_keys, TupleData *tup);
static bool build_index_scan_key(ScanKey skey, Relation rel, Relation idxrel, TupleData *tup);
static void UserTableUpdateOpenIndexes(EState *estate, TupleTableSlot *slot);
static void UserTableUpdateIndexes(EState *estate, TupleTableSlot *slot);

static bool process_remote_begin(StringInfo s);
static bool process_remote_message(StringInfo s);
static void process_remote_commit(StringInfo s);
static void process_remote_insert(StringInfo s, Relation rel);
static void process_remote_update(StringInfo s, Relation rel);
static void process_remote_delete(StringInfo s, Relation rel);

static MemoryContext TopContext;
static bool          GucAltered; /* transaction is setting some GUC variables */

/*
 * Search the index 'idxrel' for a tuple identified by 'skey' in 'rel'.
 *
 * If a matching tuple is found setup 'tid' to point to it and return true,
 * false is returned otherwise.
 */
bool
find_pkey_tuple(ScanKey skey, Relation rel, Relation idxrel,
				TupleTableSlot *slot, bool lock, LockTupleMode mode)
{
	HeapTuple	scantuple;
	bool		found;
	IndexScanDesc scan;
	SnapshotData snap;
	TransactionId xwait;

	InitDirtySnapshot(snap);
	scan = index_beginscan(rel, idxrel,
						   &snap,
						   RelationGetNumberOfAttributes(idxrel),
						   0);

retry:
	found = false;

	index_rescan(scan, skey, RelationGetNumberOfAttributes(idxrel), NULL, 0);

	if ((scantuple = index_getnext(scan, ForwardScanDirection)) != NULL)
	{
		found = true;
		/* FIXME: Improve TupleSlot to not require copying the whole tuple */
		ExecStoreTuple(scantuple, slot, InvalidBuffer, false);
		ExecMaterializeSlot(slot);

		xwait = TransactionIdIsValid(snap.xmin) ?
			snap.xmin : snap.xmax;

		if (TransactionIdIsValid(xwait))
		{
			XactLockTableWait(xwait, NULL, NULL, XLTW_None);
			goto retry;
		}
	}

	if (lock && found)
	{
		Buffer buf;
		HeapUpdateFailureData hufd;
		HTSU_Result res;
		HeapTupleData locktup;

		ItemPointerCopy(&slot->tts_tuple->t_self, &locktup.t_self);

		PushActiveSnapshot(GetLatestSnapshot());

		res = heap_lock_tuple(rel, &locktup, GetCurrentCommandId(false), mode,
							  false /* wait */,
							  false /* don't follow updates */,
							  &buf, &hufd);
		/* the tuple slot already has the buffer pinned */
		ReleaseBuffer(buf);

		PopActiveSnapshot();

		switch (res)
		{
			case HeapTupleMayBeUpdated:
				break;
			case HeapTupleUpdated:
				/* XXX: Improve handling here */
				ereport(LOG,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("concurrent update, retrying")));
				goto retry;
			default:
				elog(ERROR, "unexpected HTSU_Result after locking: %u", res);
				break;
		}
	}

	index_endscan(scan);

	return found;
}

static void
build_index_scan_keys(EState *estate, ScanKey *scan_keys, TupleData *tup)
{
	ResultRelInfo *relinfo;
	int i;

	relinfo = estate->es_result_relation_info;

	/* build scankeys for each index */
	for (i = 0; i < relinfo->ri_NumIndices; i++)
	{
		IndexInfo  *ii = relinfo->ri_IndexRelationInfo[i];

		/*
		 * Only unique indexes are of interest here, and we can't deal with
		 * expression indexes so far. FIXME: predicates should be handled
		 * better.
		 */
		if (!ii->ii_Unique || ii->ii_Expressions != NIL)
		{
			scan_keys[i] = NULL;
			continue;
		}

		scan_keys[i] = palloc(ii->ii_NumIndexAttrs * sizeof(ScanKeyData));

		/*
		 * Only return index if we could build a key without NULLs.
		 */
		if (build_index_scan_key(scan_keys[i],
								  relinfo->ri_RelationDesc,
								  relinfo->ri_IndexRelationDescs[i],
								  tup))
		{
			pfree(scan_keys[i]);
			scan_keys[i] = NULL;
			continue;
		}
	}
}

/*
 * Setup a ScanKey for a search in the relation 'rel' for a tuple 'key' that
 * is setup to match 'rel' (*NOT* idxrel!).
 *
 * Returns whether any column contains NULLs.
 */
static bool
build_index_scan_key(ScanKey skey, Relation rel, Relation idxrel, TupleData *tup)
{
	int			attoff;
	Datum		indclassDatum;
	Datum		indkeyDatum;
	bool		isnull;
	oidvector  *opclass;
	int2vector  *indkey;
	bool		hasnulls = false;

	indclassDatum = SysCacheGetAttr(INDEXRELID, idxrel->rd_indextuple,
									Anum_pg_index_indclass, &isnull);
	Assert(!isnull);
	opclass = (oidvector *) DatumGetPointer(indclassDatum);

	indkeyDatum = SysCacheGetAttr(INDEXRELID, idxrel->rd_indextuple,
									Anum_pg_index_indkey, &isnull);
	Assert(!isnull);
	indkey = (int2vector *) DatumGetPointer(indkeyDatum);


	for (attoff = 0; attoff < RelationGetNumberOfAttributes(idxrel); attoff++)
	{
		Oid			operator;
		Oid			opfamily;
		RegProcedure regop;
		int			pkattno = attoff + 1;
		int			mainattno = indkey->values[attoff];
		Oid			atttype = attnumTypeId(rel, mainattno);
		Oid			optype = get_opclass_input_type(opclass->values[attoff]);

		opfamily = get_opclass_family(opclass->values[attoff]);

		operator = get_opfamily_member(opfamily, optype,
									   optype,
									   BTEqualStrategyNumber);

		if (!OidIsValid(operator))
			elog(ERROR,
				 "could not lookup equality operator for type %u, optype %u in opfamily %u",
				 atttype, optype, opfamily);

		regop = get_opcode(operator);

		/* FIXME: convert type? */
		ScanKeyInit(&skey[attoff],
					pkattno,
					BTEqualStrategyNumber,
					regop,
					tup->values[mainattno - 1]);

		if (tup->isnull[mainattno - 1])
		{
			hasnulls = true;
			skey[attoff].sk_flags |= SK_ISNULL;
		}
	}
	return hasnulls;
}

static void
UserTableUpdateIndexes(EState *estate, TupleTableSlot *slot)
{
	/* HOT update does not require index inserts */
	if (HeapTupleIsHeapOnly(slot->tts_tuple))
		return;

	ExecOpenIndices(estate->es_result_relation_info, false);
	UserTableUpdateOpenIndexes(estate, slot);
	ExecCloseIndices(estate->es_result_relation_info);
}

static void
UserTableUpdateOpenIndexes(EState *estate, TupleTableSlot *slot)
{
	List	   *recheckIndexes = NIL;

	/* HOT update does not require index inserts */
	if (HeapTupleIsHeapOnly(slot->tts_tuple))
		return;

	if (estate->es_result_relation_info->ri_NumIndices > 0)
	{
		recheckIndexes = ExecInsertIndexTuples(slot,
											   &slot->tts_tuple->t_self,
											   estate, false, NULL, NIL);

		if (recheckIndexes != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("bdr doesn't support index rechecks")));
	}

	/* FIXME: recheck the indexes */
	list_free(recheckIndexes);
}

static EState *
create_rel_estate(Relation rel)
{
	EState	   *estate;
	ResultRelInfo *resultRelInfo;

	estate = CreateExecutorState();

	resultRelInfo = makeNode(ResultRelInfo);
	resultRelInfo->ri_RangeTableIndex = 1;		/* dummy */
	resultRelInfo->ri_RelationDesc = rel;
	resultRelInfo->ri_TrigInstrument = NULL;

	estate->es_result_relations = resultRelInfo;
	estate->es_num_result_relations = 1;
	estate->es_result_relation_info = resultRelInfo;

	return estate;
}

static bool
process_remote_begin(StringInfo s)
{
	GlobalTransactionId gtid;
	csn_t snapshot;
	int rc;

	gtid.node = pq_getmsgint(s, 4); 
	gtid.xid = pq_getmsgint(s, 4); 
	snapshot = pq_getmsgint64(s);    

	Assert(gtid.node > 0);

	MTM_LOG2("REMOTE begin node=%d xid=%d snapshot=%ld", gtid.node, gtid.xid, snapshot);
	MtmResetTransaction();		
#if 1
	if (BIT_CHECK(Mtm->disabledNodeMask, gtid.node-1)) { 
		elog(WARNING, "Ignore transaction %d from disabled node %d", gtid.xid, gtid.node);
		return false;
	}
#endif
    SetCurrentStatementStartTimestamp();     
	StartTransactionCommand();
    MtmJoinTransaction(&gtid, snapshot);

	if (GucAltered) {
		SPI_connect();
		GucAltered = false;
		rc = SPI_execute("RESET SESSION AUTHORIZATION; reset all;", false, 0);
		SPI_finish();
		if (rc < 0) { 
			elog(ERROR, "Failed to set reset context: %d", rc);
		}
	}

	return true;
}

static bool
process_remote_message(StringInfo s)
{
	char action = pq_getmsgbyte(s);
	int messageSize = pq_getmsgint(s, 4);
	char const* messageBody = pq_getmsgbytes(s, messageSize);
	bool standalone = false;

	switch (action)
	{
		case 'C':
		{
			MTM_LOG1("%d: Executing non-tx utility statement %s", MyProcPid, messageBody);
			SetCurrentStatementStartTimestamp();
			StartTransactionCommand();
			standalone = true;
			/* intentional falldown to the next case */
		}
		case 'D':
		{
			int rc;
			GucAltered = true;
			MTM_LOG1("%d: Executing utility statement %s", MyProcPid, messageBody);
			SPI_connect();
			ActivePortal->sourceText = messageBody;
			MtmVacuumStmt = NULL;
			MtmIndexStmt = NULL;
			MtmDropStmt = NULL;
			rc = SPI_execute(messageBody, false, 0);
			SPI_finish();
			if (rc < 0) { 
				elog(ERROR, "Failed to execute utility statement %s", messageBody);
			} else { 
				PushActiveSnapshot(GetTransactionSnapshot());

				if (MtmVacuumStmt != NULL) { 
					ExecVacuum(MtmVacuumStmt, 1);
				} else if (MtmIndexStmt != NULL) {
					MemoryContext oldContext = MemoryContextSwitchTo(MtmApplyContext);
					Oid relid =	RangeVarGetRelidExtended(MtmIndexStmt->relation, ShareUpdateExclusiveLock,
														 false, false,
														 NULL,
														 NULL);
					
					/* Run parse analysis ... */
					MtmIndexStmt = transformIndexStmt(relid, MtmIndexStmt, messageBody);

					MemoryContextSwitchTo(oldContext);

					DefineIndex(relid,		/* OID of heap relation */
								MtmIndexStmt,
								InvalidOid, /* no predefined OID */
								false,		/* is_alter_table */
								true,		/* check_rights */
								false,		/* skip_build */
								false);		/* quiet */
					
				} else if (MtmDropStmt != NULL) { 
					RemoveObjects(MtmDropStmt);
				}
				if (ActiveSnapshotSet())
					PopActiveSnapshot();
			}
			if (standalone) { 
				CommitTransactionCommand();
			}
			break;
		}
	    case 'A':
		{
			MtmAbortLogicalMessage* msg = (MtmAbortLogicalMessage*)messageBody;
			int origin_node = msg->origin_node;
			Assert(messageSize == sizeof(MtmAbortLogicalMessage));
			/* This function is called directly by receiver, so there is no race condition and we can update
			 * restartLSN without locks
			 */
			if (origin_node == MtmReplicationNodeId) { 
				Assert(msg->origin_lsn == InvalidXLogRecPtr);
				msg->origin_lsn = MtmSenderWalEnd;
			}
			if (Mtm->nodes[origin_node-1].restartLSN < msg->origin_lsn) { 
				MTM_LOG1("Receive logical abort message for transaction %s from node %d: %lx < %lx", msg->gid, origin_node, Mtm->nodes[origin_node-1].restartLSN, msg->origin_lsn);
				Mtm->nodes[origin_node-1].restartLSN = msg->origin_lsn;
				replorigin_session_origin_lsn = msg->origin_lsn; 				
				MtmRollbackPreparedTransaction(origin_node, msg->gid);
			} else { 
				if (msg->origin_lsn != InvalidXLogRecPtr) { 
					MTM_LOG1("Ignore rollback of transaction %s from node %d because it's LSN %lx <= %lx", 
							 msg->gid, origin_node, msg->origin_lsn, Mtm->nodes[origin_node-1].restartLSN);
				}
			}
			standalone = true;
			break;
		}
		case 'L':
		{
			MTM_LOG3("%ld: Process deadlock message with size %d from %d", MtmGetSystemTime(), messageSize, MtmReplicationNodeId);
			MtmUpdateLockGraph(MtmReplicationNodeId, messageBody, messageSize);
			standalone = true;
			break;
		}

	}
	return standalone;
}
	
static void
read_tuple_parts(StringInfo s, Relation rel, TupleData *tup)
{
	TupleDesc	desc = RelationGetDescr(rel);
	int			i;
	int			rnatts;
	char		action;

	action = pq_getmsgbyte(s);

	if (action != 'T')
		elog(ERROR, "expected TUPLE, got %c", action);

	memset(tup->isnull, 1, sizeof(tup->isnull));
	memset(tup->changed, 1, sizeof(tup->changed));

	rnatts = pq_getmsgint(s, 2);

	if (desc->natts < rnatts)
		elog(ERROR, "tuple natts mismatch, %u vs %u", desc->natts, rnatts);

	/* FIXME: unaligned data accesses */

	for (i = 0; i < desc->natts; i++)
	{
		Form_pg_attribute att = desc->attrs[i];
		char		kind;
		const char *data;
		int			len;

		if (att->atttypid == InvalidOid) { 
			continue;
		}

		kind = pq_getmsgbyte(s);

		switch (kind)
		{
			case 'n': /* null */
				/* already marked as null */
				tup->values[i] = 0xdeadbeef;
				break;
			case 'u': /* unchanged column */
				tup->isnull[i] = true;
				tup->changed[i] = false;
				tup->values[i] = 0xdeadbeef; /* make bad usage more obvious */
				break;

			case 'b': /* binary format */
				tup->isnull[i] = false;
				len = pq_getmsgint(s, 4); /* read length */

				data = pq_getmsgbytes(s, len);

				/* and data */
				if (att->attbyval)
					tup->values[i] = fetch_att(data, true, len);
				else
					tup->values[i] = PointerGetDatum(data);
				break;
			case 's': /* send/recv format */
				{
					Oid typreceive;
					Oid typioparam;
					StringInfoData buf;

					tup->isnull[i] = false;
					len = pq_getmsgint(s, 4); /* read length */

					getTypeBinaryInputInfo(att->atttypid,
										   &typreceive, &typioparam);

					/* create StringInfo pointing into the bigger buffer */
					initStringInfo(&buf);
					/* and data */
					buf.data = (char *) pq_getmsgbytes(s, len);
					buf.len = len;
					tup->values[i] = OidReceiveFunctionCall(
						typreceive, &buf, typioparam, att->atttypmod);

					if (buf.len != buf.cursor)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
								 errmsg("incorrect binary data format")));
					break;
				}
			case 't': /* text format */
				{
					Oid typinput;
					Oid typioparam;

					tup->isnull[i] = false;
					len = pq_getmsgint(s, 4); /* read length */

					getTypeInputInfo(att->atttypid, &typinput, &typioparam);
					/* and data */
					data = (char *) pq_getmsgbytes(s, len);
					tup->values[i] = OidInputFunctionCall(
						typinput, (char *) data, typioparam, att->atttypmod);
				}
				break;
			default:
				elog(ERROR, "unknown column type '%c'", kind);
		}

		if (att->attisdropped && !tup->isnull[i])
			elog(ERROR, "data for dropped column");
	}
}

static Relation 
read_rel(StringInfo s, LOCKMODE mode)
{
	int			relnamelen;
	int			nspnamelen;
	RangeVar*	rv;
	Oid			remote_relid = pq_getmsgint(s, 4);
	Oid         local_relid;

	local_relid = pglogical_relid_map_get(remote_relid);
	if (local_relid == InvalidOid) { 
		rv = makeNode(RangeVar);

		nspnamelen = pq_getmsgbyte(s);
		rv->schemaname = (char *) pq_getmsgbytes(s, nspnamelen);
		
		relnamelen = pq_getmsgbyte(s);
		rv->relname = (char *) pq_getmsgbytes(s, relnamelen);
		
		local_relid = RangeVarGetRelidExtended(rv, mode, false, false, NULL, NULL);
		pglogical_relid_map_put(remote_relid, local_relid);
	} else { 
		nspnamelen = pq_getmsgbyte(s);
		s->cursor += nspnamelen;
		relnamelen = pq_getmsgbyte(s);
		s->cursor += relnamelen;
	}
	return heap_open(local_relid, NoLock);
}

static void
process_remote_commit(StringInfo in)
{
	uint8 		flags;
	csn_t       csn;
	const char *gid = NULL;	
	XLogRecPtr  end_lsn;
	XLogRecPtr  origin_lsn;
	int         origin_node;
	/* read flags */
	flags = pq_getmsgbyte(in);
	MtmReplicationNodeId = pq_getmsgbyte(in);

	/* read fields */
	pq_getmsgint64(in); /* commit_lsn */
	end_lsn = pq_getmsgint64(in); /* end_lsn */
	replorigin_session_origin_timestamp = pq_getmsgint64(in); /* commit_time */

	origin_node = pq_getmsgbyte(in);
	origin_lsn = pq_getmsgint64(in);

	replorigin_session_origin_lsn = origin_node == MtmReplicationNodeId ? end_lsn : origin_lsn;
	Assert(replorigin_session_origin == InvalidRepOriginId);

	switch (PGLOGICAL_XACT_EVENT(flags))
	{
	    case PGLOGICAL_PRECOMMIT_PREPARED:
		{
			Assert(!TransactionIdIsValid(MtmGetCurrentTransactionId()));
			gid = pq_getmsgstring(in);
			MTM_LOG2("%d: PGLOGICAL_PRECOMMIT_PREPARED %s", MyProcPid, gid);
			MtmBeginSession(origin_node);
			MtmPrecommitTransaction(gid);
			MtmEndSession(origin_node, true);
			return;
		}
		case PGLOGICAL_COMMIT:
		{
			MTM_LOG2("%d: PGLOGICAL_COMMIT commit", MyProcPid);
			if (IsTransactionState()) {
				Assert(TransactionIdIsValid(MtmGetCurrentTransactionId()));
				MtmBeginSession(origin_node);
				CommitTransactionCommand();
				MtmEndSession(origin_node, true);
			}
			break;
		}
		case PGLOGICAL_PREPARE:
		{
			Assert(IsTransactionState() && TransactionIdIsValid(MtmGetCurrentTransactionId()));
			gid = pq_getmsgstring(in);
			if (MtmExchangeGlobalTransactionStatus(gid, TRANSACTION_STATUS_IN_PROGRESS) == TRANSACTION_STATUS_ABORTED) { 
				MTM_LOG1("Avoid prepare of previously aborted global transaction %s", gid);	
				AbortCurrentTransaction();
			} else { 				
				/* prepare TBLOCK_INPROGRESS state for PrepareTransactionBlock() */
				MTM_LOG2("PGLOGICAL_PREPARE commit: gid=%s", gid);
				BeginTransactionBlock();
				CommitTransactionCommand();
				StartTransactionCommand();
				
				MtmBeginSession(origin_node);
				/* PREPARE itself */
				MtmSetCurrentTransactionGID(gid);
				PrepareTransactionBlock(gid);
				CommitTransactionCommand();

				if (MtmExchangeGlobalTransactionStatus(gid, TRANSACTION_STATUS_UNKNOWN) == TRANSACTION_STATUS_ABORTED) { 
					MTM_LOG1("Perform delayed rollback of prepared global transaction %s", gid);	
					StartTransactionCommand();
					MtmSetCurrentTransactionGID(gid);
					FinishPreparedTransaction(gid, false);
					CommitTransactionCommand();					
				}	
				MtmEndSession(origin_node, true);
			}
			break;
		}
		case PGLOGICAL_COMMIT_PREPARED:
		{
			Assert(!TransactionIdIsValid(MtmGetCurrentTransactionId()));
			csn = pq_getmsgint64(in); 
			gid = pq_getmsgstring(in);
			MTM_LOG2("PGLOGICAL_COMMIT_PREPARED commit: csn=%ld, gid=%s, lsn=%lx", csn, gid, end_lsn);
			MtmResetTransaction();
			StartTransactionCommand();
			MtmBeginSession(origin_node);
			MtmSetCurrentTransactionCSN(csn);
			MtmSetCurrentTransactionGID(gid);
			FinishPreparedTransaction(gid, true);
			CommitTransactionCommand();
			MtmEndSession(origin_node, true);
			break;
		}
		case PGLOGICAL_ABORT_PREPARED:
		{
			Assert(!TransactionIdIsValid(MtmGetCurrentTransactionId()));
			gid = pq_getmsgstring(in);
			/* MtmRollbackPreparedTransaction will set origin session itself */
			MTM_LOG1("Receive ABORT_PREPARED logical message for transaction %s from node %d", gid, origin_node);
			MtmRollbackPreparedTransaction(origin_node, gid);
			break;
		}
		default:
			Assert(false);
	}
	if (Mtm->status == MTM_RECOVERY) { 
		MTM_LOG1("Recover transaction %s flags=%d", gid,  flags);
	}
	MtmUpdateLsnMapping(MtmReplicationNodeId, end_lsn);
	if (flags & PGLOGICAL_CAUGHT_UP) {
		MtmRecoveryCompleted();
	}
}

static void
process_remote_insert(StringInfo s, Relation rel)
{
	EState *estate;
	TupleData new_tuple;
	TupleTableSlot *newslot;
	TupleTableSlot *oldslot;
	ResultRelInfo *relinfo;
	ScanKey	*index_keys;
	int	i;

	PushActiveSnapshot(GetTransactionSnapshot());

	estate = create_rel_estate(rel);
	newslot = ExecInitExtraTupleSlot(estate);
	oldslot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(newslot, RelationGetDescr(rel));
	ExecSetSlotDescriptor(oldslot, RelationGetDescr(rel));

	read_tuple_parts(s, rel, &new_tuple);
	{
		HeapTuple tup;
		tup = heap_form_tuple(RelationGetDescr(rel),
							  new_tuple.values, new_tuple.isnull);
		ExecStoreTuple(tup, newslot, InvalidBuffer, true);
	}

	// if (rel->rd_rel->relkind != RELKIND_RELATION) // RELKIND_MATVIEW
	// 	elog(ERROR, "unexpected relkind '%c' rel \"%s\"",
	// 		 rel->rd_rel->relkind, RelationGetRelationName(rel));

	/* debug output */
#ifdef VERBOSE_INSERT
	log_tuple("INSERT:%s", RelationGetDescr(rel), newslot->tts_tuple);
#endif

	/*
	 * Search for conflicting tuples.
	 */
	ExecOpenIndices(estate->es_result_relation_info, false);
	relinfo = estate->es_result_relation_info;
	index_keys = palloc0(relinfo->ri_NumIndices * sizeof(ScanKeyData*));

	build_index_scan_keys(estate, index_keys, &new_tuple);

	/* do a SnapshotDirty search for conflicting tuples */
	for (i = 0; i < relinfo->ri_NumIndices; i++)
	{
		IndexInfo  *ii = relinfo->ri_IndexRelationInfo[i];
		bool found = false;

		/*
		 * Only unique indexes are of interest here, and we can't deal with
		 * expression indexes so far. FIXME: predicates should be handled
		 * better.
		 *
		 * NB: Needs to match expression in build_index_scan_key
		 */
		if (!ii->ii_Unique || ii->ii_Expressions != NIL)
			continue;

		if (index_keys[i] == NULL)
			continue;

		Assert(ii->ii_Expressions == NIL);

		/* if conflict: wait */
		found = find_pkey_tuple(index_keys[i],
								rel, relinfo->ri_IndexRelationDescs[i],
								oldslot, true, LockTupleExclusive);

		/* alert if there's more than one conflicting unique key */
		if (found)
		{
			/* TODO: Report tuple identity in log */
			ereport(ERROR,
                    (errcode(ERRCODE_UNIQUE_VIOLATION),
                     errmsg("Unique constraints violated by remotely INSERTed tuple"),
                     errdetail("Cannot apply transaction because remotely INSERTed tuple conflicts with a local tuple on UNIQUE constraint and/or PRIMARY KEY")));
		}
		CHECK_FOR_INTERRUPTS();
	}

	simple_heap_insert(rel, newslot->tts_tuple);
    UserTableUpdateOpenIndexes(estate, newslot);

	ExecCloseIndices(estate->es_result_relation_info);

	if (ActiveSnapshotSet())
		PopActiveSnapshot();

    heap_close(rel, NoLock);
    ExecResetTupleTable(estate->es_tupleTable, true);
    FreeExecutorState(estate);

	CommandCounterIncrement();
}

static void
process_remote_update(StringInfo s, Relation rel)
{
	char		action;
	EState	   *estate;
	TupleTableSlot *newslot;
	TupleTableSlot *oldslot;
	bool		pkey_sent;
	bool		found_tuple;
	TupleData   old_tuple;
	TupleData   new_tuple;
	Oid			idxoid;
	Relation	idxrel;
	ScanKeyData skey[INDEX_MAX_KEYS];
	HeapTuple	remote_tuple = NULL;

	action = pq_getmsgbyte(s);

	/* old key present, identifying key changed */
	if (action != 'K' && action != 'N')
		elog(ERROR, "expected action 'N' or 'K', got %c",
			 action);

	estate = create_rel_estate(rel);
	oldslot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(oldslot, RelationGetDescr(rel));
	newslot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(newslot, RelationGetDescr(rel));

	if (action == 'K')
	{
		pkey_sent = true;
		read_tuple_parts(s, rel, &old_tuple);
		action = pq_getmsgbyte(s);
	}
	else
		pkey_sent = false;

	/* check for new  tuple */
	if (action != 'N')
		elog(ERROR, "expected action 'N', got %c",
			 action);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "unexpected relkind '%c' rel \"%s\"",
			 rel->rd_rel->relkind, RelationGetRelationName(rel));

	/* read new tuple */
	read_tuple_parts(s, rel, &new_tuple);

	/* lookup index to build scankey */
	if (rel->rd_indexvalid == 0)
		RelationGetIndexList(rel);
	idxoid = rel->rd_replidindex;
	if (!OidIsValid(idxoid))
	{
		elog(ERROR, "could not find primary key for table with oid %u",
			 RelationGetRelid(rel));
		return;
	}

	/* open index, so we can build scan key for row */
	idxrel = index_open(idxoid, RowExclusiveLock);

	Assert(idxrel->rd_index->indisunique);

	/* Use columns from the new tuple if the key didn't change. */
	build_index_scan_key(skey, rel, idxrel,
						 pkey_sent ? &old_tuple : &new_tuple);

	PushActiveSnapshot(GetTransactionSnapshot());

	/* look for tuple identified by the (old) primary key */
	found_tuple = find_pkey_tuple(skey, rel, idxrel, oldslot, true,
						pkey_sent ? LockTupleExclusive : LockTupleNoKeyExclusive);

	if (found_tuple)
	{
		remote_tuple = heap_modify_tuple(oldslot->tts_tuple,
										 RelationGetDescr(rel),
										 new_tuple.values,
										 new_tuple.isnull,
										 new_tuple.changed);

		ExecStoreTuple(remote_tuple, newslot, InvalidBuffer, true);

#ifdef VERBOSE_UPDATE
		{
			StringInfoData o;
			initStringInfo(&o);
			tuple_to_stringinfo(&o, RelationGetDescr(rel), oldslot->tts_tuple);
			appendStringInfo(&o, " to");
			tuple_to_stringinfo(&o, RelationGetDescr(rel), remote_tuple);
			MTM_LOG1(DEBUG1, "UPDATE:%s", o.data);
			resetStringInfo(&o);
		}
#endif

        simple_heap_update(rel, &oldslot->tts_tuple->t_self, newslot->tts_tuple);
        UserTableUpdateIndexes(estate, newslot);
	}
	else
	{
        ereport(ERROR,
                (errcode(ERRCODE_NO_DATA_FOUND),
                 errmsg("Record with specified key can not be located at this node"),
                 errdetail("Most likely we have DELETE-UPDATE conflict")));

	}
    
	PopActiveSnapshot();
    
	/* release locks upon commit */
	index_close(idxrel, NoLock);
	heap_close(rel, NoLock);
    
	ExecResetTupleTable(estate->es_tupleTable, true);
	FreeExecutorState(estate);

	CommandCounterIncrement();
}

static void
process_remote_delete(StringInfo s, Relation rel)
{
	EState	   *estate;
	TupleData   oldtup;
	TupleTableSlot *oldslot;
	Oid			idxoid;
	Relation	idxrel;
	ScanKeyData skey[INDEX_MAX_KEYS];
	bool		found_old;

	estate = create_rel_estate(rel);
	oldslot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(oldslot, RelationGetDescr(rel));

	read_tuple_parts(s, rel, &oldtup);

	/* lookup index to build scankey */
	if (rel->rd_indexvalid == 0)
		RelationGetIndexList(rel);
	idxoid = rel->rd_replidindex;
	if (!OidIsValid(idxoid))
	{
		elog(ERROR, "could not find primary key for table with oid %u",
			 RelationGetRelid(rel));
		return;
	}

	/* Now open the primary key index */
	idxrel = index_open(idxoid, RowExclusiveLock);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "unexpected relkind '%c' rel \"%s\"",
			 rel->rd_rel->relkind, RelationGetRelationName(rel));

#ifdef VERBOSE_DELETE
	{
		HeapTuple tup;
		tup = heap_form_tuple(RelationGetDescr(rel),
							  oldtup.values, oldtup.isnull);
		ExecStoreTuple(tup, oldslot, InvalidBuffer, true);
	}
	log_tuple("DELETE old-key:%s", RelationGetDescr(rel), oldslot->tts_tuple);
#endif

	PushActiveSnapshot(GetTransactionSnapshot());

	build_index_scan_key(skey, rel, idxrel, &oldtup);

	/* try to find tuple via a (candidate|primary) key */
	found_old = find_pkey_tuple(skey, rel, idxrel, oldslot, true, LockTupleExclusive);

	if (found_old)
	{
		simple_heap_delete(rel, &oldslot->tts_tuple->t_self);
	}
	else
	{
        ereport(ERROR,
                (errcode(ERRCODE_NO_DATA_FOUND),
                 errmsg("Record with specified key can not be located at this node"),
                 errdetail("Most likely we have DELETE-DELETE conflict")));
	}

	PopActiveSnapshot();

	index_close(idxrel, NoLock);
	heap_close(rel, NoLock);

	ExecResetTupleTable(estate->es_tupleTable, true);
	FreeExecutorState(estate);

	CommandCounterIncrement();
}

void MtmExecutor(void* work, size_t size)
{
    StringInfoData s;
    Relation rel = NULL;
	int spill_file = -1;
	int save_cursor = 0;
	int save_len = 0;
    s.data = work;
    s.len = size;
    s.maxlen = -1;
	s.cursor = 0;

    if (MtmApplyContext == NULL) {
        MtmApplyContext = AllocSetContextCreate(TopMemoryContext,
												"ApplyContext",
												ALLOCSET_DEFAULT_MINSIZE,
												ALLOCSET_DEFAULT_INITSIZE,
												ALLOCSET_DEFAULT_MAXSIZE);
    }
    TopContext = MemoryContextSwitchTo(MtmApplyContext);
	replorigin_session_origin = InvalidRepOriginId;
    PG_TRY();
    {    
        while (true) { 
            char action = pq_getmsgbyte(&s);
            MTM_LOG2("%d: REMOTE process action %c", MyProcPid, action);
#if 0
			if (Mtm->status == MTM_RECOVERY) { 
				MTM_LOG1("Replay action %c[%x]",   action, s.data[s.cursor]);
			}
#endif

            switch (action) {
                /* BEGIN */
            case 'B':
			    if (process_remote_begin(&s)) { 				   
					continue;
				} else { 
					break;
				}
                /* COMMIT */
            case 'C':
                process_remote_commit(&s);
                break;
                /* INSERT */
            case 'I':
                process_remote_insert(&s, rel);
                continue;
                /* UPDATE */
            case 'U':
                process_remote_update(&s, rel);
                continue;
                /* DELETE */
            case 'D':
                process_remote_delete(&s, rel);
                continue;
            case 'R':
                rel = read_rel(&s, RowExclusiveLock);
                continue;
			case 'F':
			{
				int node_id = pq_getmsgint(&s, 4);
				int file_id = pq_getmsgint(&s, 4);
				Assert(spill_file < 0);
				spill_file = MtmOpenSpillFile(node_id, file_id);
				continue;
			}
 		    case '(':
			{
			    int64 size = pq_getmsgint(&s, 4);    
				s.data = palloc(size);
				save_cursor = s.cursor;
				save_len = s.len;
				s.cursor = 0;
				s.len = size;
				MtmReadSpillFile(spill_file, s.data, size);
				continue;
			}
  		    case ')':
			{
  			    pfree(s.data);
				s.data = work;
  			    s.cursor = save_cursor;
				s.len = save_len;
				continue;
			}
			case 'M':
			{
				if (process_remote_message(&s)) { 
					break;
				}
				continue;
			}
            default:
                elog(ERROR, "unknown action of type %c", action);
            }        
            break;
        }
    }
    PG_CATCH();
    {
		MemoryContext oldcontext = MemoryContextSwitchTo(MtmApplyContext);
		MtmHandleApplyError();
		MemoryContextSwitchTo(oldcontext);
		EmitErrorReport();
        FlushErrorState();
		MTM_LOG1("%d: REMOTE begin abort transaction %d", MyProcPid, MtmGetCurrentTransactionId());
		MtmEndSession(MtmReplicationNodeId, false);
        AbortCurrentTransaction();
		MTM_LOG2("%d: REMOTE end abort transaction %d", MyProcPid, MtmGetCurrentTransactionId());
    }
    PG_END_TRY();
	if (spill_file >= 0) { 
		MtmCloseSpillFile(spill_file);
	}
    MemoryContextResetAndDeleteChildren(MtmApplyContext);
}
    

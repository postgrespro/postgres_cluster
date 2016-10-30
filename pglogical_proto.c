#include "postgres.h"

#include "miscadmin.h"

#include "pglogical_output.h"
#include "replication/origin.h"

#include "access/sysattr.h"
#include "access/tuptoaster.h"
#include "access/xact.h"
#include "access/clog.h"

#include "catalog/catversion.h"
#include "catalog/index.h"

#include "catalog/namespace.h"
#include "catalog/pg_class.h"
#include "catalog/pg_database.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_type.h"

#include "commands/dbcommands.h"

#include "executor/spi.h"

#include "libpq/pqformat.h"

#include "mb/pg_wchar.h"

#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/typcache.h"

#include "multimaster.h"
#include "pglogical_relid_map.h"

static int MtmTransactionRecords;
static TransactionId MtmCurrentXid;
static bool DDLInProress = false;

static void pglogical_write_rel(StringInfo out, PGLogicalOutputData *data, Relation rel);

static void pglogical_write_begin(StringInfo out, PGLogicalOutputData *data,
							ReorderBufferTXN *txn);
static void pglogical_write_commit(StringInfo out,PGLogicalOutputData *data,
							ReorderBufferTXN *txn, XLogRecPtr commit_lsn);

static void pglogical_write_insert(StringInfo out, PGLogicalOutputData *data,
							Relation rel, HeapTuple newtuple);
static void pglogical_write_update(StringInfo out, PGLogicalOutputData *data,
							Relation rel, HeapTuple oldtuple,
							HeapTuple newtuple);
static void pglogical_write_delete(StringInfo out, PGLogicalOutputData *data,
							Relation rel, HeapTuple oldtuple);

static void pglogical_write_tuple(StringInfo out, PGLogicalOutputData *data,
								   Relation rel, HeapTuple tuple);
static char decide_datum_transfer(Form_pg_attribute att,
								  Form_pg_type typclass,
								  bool allow_internal_basetypes,
								  bool allow_binary_basetypes);

/*
 * Write relation description to the output stream.
 */
static void
pglogical_write_rel(StringInfo out, PGLogicalOutputData *data, Relation rel)
{
	const char *nspname;
	uint8		nspnamelen;
	const char *relname;
	uint8		relnamelen;
	Oid         relid;

	if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN) {
		MTM_LOG2("%d: pglogical_write_message filtered", MyProcPid);
		return;
	}

	if (DDLInProress) {
		MTM_LOG2("%d: pglogical_write_message filtered DDLInProress", MyProcPid);
		return;
	}

	relid = RelationGetRelid(rel);
	pq_sendbyte(out, 'R');		/* sending RELATION */	
	pq_sendint(out, relid, sizeof relid); /* use Oid as relation identifier */
	
	nspname = get_namespace_name(rel->rd_rel->relnamespace);
	if (nspname == NULL)
		elog(ERROR, "cache lookup failed for namespace %u",
				 rel->rd_rel->relnamespace);
	nspnamelen = strlen(nspname) + 1;
	
	relname = NameStr(rel->rd_rel->relname);
	relnamelen = strlen(relname) + 1;
	
	pq_sendbyte(out, nspnamelen);		/* schema name length */
	pq_sendbytes(out, nspname, nspnamelen);
	
	pq_sendbyte(out, relnamelen);		/* table name length */
	pq_sendbytes(out, relname, relnamelen);
}

/*
 * Write BEGIN to the output stream.
 */
static void
pglogical_write_begin(StringInfo out, PGLogicalOutputData *data,
					  ReorderBufferTXN *txn)
{
	bool isRecovery = MtmIsRecoveredNode(MtmReplicationNodeId);
	csn_t csn = MtmTransactionSnapshot(txn->xid);

	MtmCurrentXid = txn->xid;

	MTM_LOG3("%d: pglogical_write_begin XID=%d node=%d CSN=%ld recovery=%d restart_decoding_lsn=%lx first_lsn=%lx end_lsn=%lx confirmed_flush=%lx", 
			 MyProcPid, txn->xid, MtmReplicationNodeId, csn, isRecovery, txn->restart_decoding_lsn, txn->first_lsn, txn->end_lsn, MyReplicationSlot->data.confirmed_flush);

	MTM_LOG3("%d: pglogical_write_begin XID=%d sent", MyProcPid, txn->xid);
	pq_sendbyte(out, 'B');		/* BEGIN */
	pq_sendint(out, MtmNodeId, 4);
	pq_sendint(out, isRecovery ? InvalidTransactionId : txn->xid, 4);
	pq_sendint64(out, csn);
	MtmTransactionRecords = 0;
}

static void
pglogical_write_message(StringInfo out,
						const char *prefix, Size sz, const char *message)
{
	switch (*prefix) { 
	  case 'L':
		if (MtmIsRecoveredNode(MtmReplicationNodeId)) { 			
			return;
		} else { 
			MTM_LOG1("Send deadlock message to node %d", MtmReplicationNodeId);
		}
		break;
	  case 'D':
		if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN)	{
			MTM_LOG2("%d: pglogical_write_message filtered", MyProcPid);
			return;
		}
		DDLInProress = true;
		break;
	  case 'E':
		DDLInProress = false;
		/*
		 * we use End message only as indicator of DDL transaction finish,
		 * so no need to send that to replicas.
		 */
		return;
	}
	pq_sendbyte(out, 'M');
	pq_sendbyte(out, *prefix);
	pq_sendint(out, sz, 4);
	pq_sendbytes(out, message, sz);
}

/*
 * Write COMMIT to the output stream.
 */
static void
pglogical_write_commit(StringInfo out, PGLogicalOutputData *data,
					   ReorderBufferTXN *txn, XLogRecPtr commit_lsn)
{
    uint8 flags = 0;
	
	MTM_LOG3("%d: pglogical_write_commit XID=%d node=%d restart_decoding_lsn=%lx first_lsn=%lx end_lsn=%lx confirmed_flush=%lx", 
			 MyProcPid, txn->xid, MtmReplicationNodeId, txn->restart_decoding_lsn, txn->first_lsn, txn->end_lsn, MyReplicationSlot->data.confirmed_flush);


    if (txn->xact_action == XLOG_XACT_COMMIT) 
    	flags = PGLOGICAL_COMMIT;
	else if (txn->xact_action == XLOG_XACT_PREPARE)
    	flags = PGLOGICAL_PREPARE;
	else if (txn->xact_action == XLOG_XACT_COMMIT_PREPARED)
    	flags = PGLOGICAL_COMMIT_PREPARED;
	else if (txn->xact_action == XLOG_XACT_ABORT_PREPARED)
    	flags = PGLOGICAL_ABORT_PREPARED;
	else
    	Assert(false);

	if (flags == PGLOGICAL_COMMIT || flags == PGLOGICAL_PREPARE) { 
		Assert(txn->xid < 1000 || MtmTransactionRecords >= 2);
		// if (MtmIsFilteredTxn) { 
			// Assert(MtmTransactionRecords == 0);
			// return;
		// }
	} else { 
		csn_t csn = MtmTransactionSnapshot(txn->xid);
		bool isRecovery = MtmIsRecoveredNode(MtmReplicationNodeId);

		if (!isRecovery && csn == INVALID_CSN && (flags != PGLOGICAL_ABORT_PREPARED || txn->origin_id != InvalidRepOriginId))
		{
			if (flags == PGLOGICAL_ABORT_PREPARED) { 
				MTM_LOG1("Skip ABORT_PREPARED for transaction %s to node %d", txn->gid, MtmReplicationNodeId);
			}
			Assert(MtmTransactionRecords == 0);
			return;
		}
		if (flags == PGLOGICAL_ABORT_PREPARED) { 
			MTM_LOG1("Send ABORT_PREPARED for transaction %d (%s) end_lsn=%lx to node %d, isRecovery=%d, txn->origin_id=%d, csn=%ld", 
					 txn->xid, txn->gid, txn->end_lsn, MtmReplicationNodeId, isRecovery, txn->origin_id, csn);
		}
		if (MtmRecoveryCaughtUp(MtmReplicationNodeId, txn->end_lsn)) { 
			MTM_LOG1("wal-sender complete recovery of node %d at LSN(commit %lx, end %lx, log %lx) in transaction %s event %d", MtmReplicationNodeId, commit_lsn, txn->end_lsn, GetXLogInsertRecPtr(), txn->gid, flags);
			flags |= PGLOGICAL_CAUGHT_UP;
		}
	}

    pq_sendbyte(out, 'C');		/* sending COMMIT */

	MTM_LOG2("PGLOGICAL_SEND commit: event=%d, gid=%s, commit_lsn=%lx, txn->end_lsn=%lx, xlog=%lx", flags, txn->gid, commit_lsn, txn->end_lsn, GetXLogInsertRecPtr());

    /* send the flags field */
    pq_sendbyte(out, flags);
    pq_sendbyte(out, MtmNodeId);

    /* send fixed fields */
    pq_sendint64(out, commit_lsn);
    pq_sendint64(out, txn->end_lsn);
    pq_sendint64(out, txn->commit_time);

	if (txn->origin_id != InvalidRepOriginId) { 
		int i;
		for (i = 0; i < Mtm->nAllNodes && Mtm->nodes[i].originId != txn->origin_id; i++);
		if (i == Mtm->nAllNodes) { 
			elog(WARNING, "Failed to map origin %d", txn->origin_id);
			i = MtmNodeId-1;
		} else { 
			Assert(i == MtmNodeId-1 || txn->origin_lsn != InvalidXLogRecPtr);
		}
		pq_sendbyte(out, i+1);
	} else { 
		pq_sendbyte(out, MtmNodeId);
	}
	pq_sendint64(out, txn->origin_lsn);

	if (txn->xact_action == XLOG_XACT_COMMIT_PREPARED) { 
		Assert(MtmTransactionRecords == 0);
		pq_sendint64(out, MtmGetTransactionCSN(txn->xid));
	}
    if (txn->xact_action != XLOG_XACT_COMMIT) { 
    	pq_sendstring(out, txn->gid);
	}

	MtmTransactionRecords = 0;
	MTM_TXTRACE(txn, "pglogical_write_commit Finish");
}

/*
 * Write INSERT to the output stream.
 */
static void
pglogical_write_insert(StringInfo out, PGLogicalOutputData *data,
						Relation rel, HeapTuple newtuple)
{
	if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN){
		MTM_LOG2("%d: pglogical_write_insert filtered", MyProcPid);
		return;
	}

	if (DDLInProress) {
		MTM_LOG2("%d: pglogical_write_insert filtered DDLInProress", MyProcPid);
		return;
	}

	MtmTransactionRecords += 1;
	pq_sendbyte(out, 'I');		/* action INSERT */
	pglogical_write_tuple(out, data, rel, newtuple);

}

/*
 * Write UPDATE to the output stream.
 */
static void
pglogical_write_update(StringInfo out, PGLogicalOutputData *data,
						Relation rel, HeapTuple oldtuple, HeapTuple newtuple)
{
	if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN){
		MTM_LOG2("%d: pglogical_write_update filtered", MyProcPid);
		return;
	}

	if (DDLInProress) {
		MTM_LOG2("%d: pglogical_write_update filtered DDLInProress", MyProcPid);
		return;
	}

	MtmTransactionRecords += 1;

	MTM_LOG3("%d: pglogical_write_update confirmed_flush=%lx", MyProcPid, MyReplicationSlot->data.confirmed_flush);

	pq_sendbyte(out, 'U');		/* action UPDATE */
	/* FIXME support whole tuple (O tuple type) */
	if (oldtuple != NULL)
	{
		pq_sendbyte(out, 'K');	/* old key follows */
		pglogical_write_tuple(out, data, rel, oldtuple);
	}

	pq_sendbyte(out, 'N');		/* new tuple follows */
	pglogical_write_tuple(out, data, rel, newtuple);
}
	
/*
 * Write DELETE to the output stream.
 */
static void
pglogical_write_delete(StringInfo out, PGLogicalOutputData *data,
						Relation rel, HeapTuple oldtuple)
{
	if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN){
		MTM_LOG2("%d: pglogical_write_delete filtered", MyProcPid);
		return;
	}

	if (DDLInProress) {
		MTM_LOG2("%d: pglogical_write_delete filtered DDLInProress", MyProcPid);
		return;
	}

	MtmTransactionRecords += 1;
	pq_sendbyte(out, 'D');		/* action DELETE */
	pglogical_write_tuple(out, data, rel, oldtuple);
}

/*
 * Most of the brains for startup message creation lives in
 * pglogical_config.c, so this presently just sends the set of key/value pairs.
 */
static void
write_startup_message(StringInfo out, List *msg)
{
}

/*
 * Write a tuple to the outputstream, in the most efficient format possible.
 */
static void
pglogical_write_tuple(StringInfo out, PGLogicalOutputData *data,
					   Relation rel, HeapTuple tuple)
{
	TupleDesc	desc;
	Datum		values[MaxTupleAttributeNumber];
	bool		isnull[MaxTupleAttributeNumber];
	int			i;
	uint16		nliveatts = 0;

	if (MtmTransactionSnapshot(MtmCurrentXid) == INVALID_CSN){
		MTM_LOG2("%d: pglogical_write_tuple filtered", MyProcPid);
		return;
	}

	if (DDLInProress) {
		MTM_LOG2("%d: pglogical_write_tuple filtered DDLInProress", MyProcPid);
		return;
	}

	desc = RelationGetDescr(rel);

	pq_sendbyte(out, 'T');			/* sending TUPLE */

	for (i = 0; i < desc->natts; i++)
	{
		if (desc->attrs[i]->attisdropped)
			continue;
		nliveatts++;
	}
	pq_sendint(out, nliveatts, 2);

	/* try to allocate enough memory from the get go */
	enlargeStringInfo(out, tuple->t_len +
					  nliveatts * (1 + 4));

	/*
	 * XXX: should this prove to be a relevant bottleneck, it might be
	 * interesting to inline heap_deform_tuple() here, we don't actually need
	 * the information in the form we get from it.
	 */
	heap_deform_tuple(tuple, desc, values, isnull);

	for (i = 0; i < desc->natts; i++)
	{
		HeapTuple	typtup;
		Form_pg_type typclass;
		Form_pg_attribute att = desc->attrs[i];
		char		transfer_type;

		/* skip dropped columns */
		if (att->attisdropped)
			continue;

		if (isnull[i])
		{
			pq_sendbyte(out, 'n');	/* null column */
			continue;
		}
		else if (att->attlen == -1 && VARATT_IS_EXTERNAL_ONDISK(values[i]))
		{
			pq_sendbyte(out, 'u');	/* unchanged toast column */
			continue;
		}

		typtup = SearchSysCache1(TYPEOID, ObjectIdGetDatum(att->atttypid));
		if (!HeapTupleIsValid(typtup))
			elog(ERROR, "cache lookup failed for type %u", att->atttypid);
		typclass = (Form_pg_type) GETSTRUCT(typtup);

		transfer_type = decide_datum_transfer(att, typclass,
											  data->allow_internal_basetypes,
											  data->allow_binary_basetypes);
        pq_sendbyte(out, transfer_type);
		switch (transfer_type)
		{
			case 'b':	/* internal-format binary data follows */

				/* pass by value */
				if (att->attbyval)
				{
					pq_sendint(out, att->attlen, 4); /* length */

					enlargeStringInfo(out, att->attlen);
					store_att_byval(out->data + out->len, values[i],
									att->attlen);
					out->len += att->attlen;
					out->data[out->len] = '\0';
				}
				/* fixed length non-varlena pass-by-reference type */
				else if (att->attlen > 0)
				{
					pq_sendint(out, att->attlen, 4); /* length */

					appendBinaryStringInfo(out, DatumGetPointer(values[i]),
										   att->attlen);
				}
				/* varlena type */
				else if (att->attlen == -1)
				{
					char *data = DatumGetPointer(values[i]);

					/* send indirect datums inline */
					if (VARATT_IS_EXTERNAL_INDIRECT(values[i]))
					{
						struct varatt_indirect redirect;
						VARATT_EXTERNAL_GET_POINTER(redirect, data);
						data = (char *) redirect.pointer;
					}

					Assert(!VARATT_IS_EXTERNAL(data));

					pq_sendint(out, VARSIZE_ANY(data), 4); /* length */

					appendBinaryStringInfo(out, data, VARSIZE_ANY(data));
				}
				else
					elog(ERROR, "unsupported tuple type");

				break;

			case 's': /* binary send/recv data follows */
				{
					bytea	   *outputbytes;
					int			len;

					outputbytes = OidSendFunctionCall(typclass->typsend,
													  values[i]);

					len = VARSIZE(outputbytes) - VARHDRSZ;
					pq_sendint(out, len, 4); /* length */
					pq_sendbytes(out, VARDATA(outputbytes), len); /* data */
					pfree(outputbytes);
				}
				break;

			default:
				{
					char   	   *outputstr;
					int			len;

					outputstr =	OidOutputFunctionCall(typclass->typoutput,
													  values[i]);
					len = strlen(outputstr) + 1;
					pq_sendint(out, len, 4); /* length */
					appendBinaryStringInfo(out, outputstr, len); /* data */
					pfree(outputstr);
				}
		}

		ReleaseSysCache(typtup);
	}
}

/*
 * Make the executive decision about which protocol to use.
 */
static char
decide_datum_transfer(Form_pg_attribute att, Form_pg_type typclass,
					  bool allow_internal_basetypes,
					  bool allow_binary_basetypes)
{
	/*
	 * Use the binary protocol, if allowed, for builtin & plain datatypes.
	 */
	if (allow_internal_basetypes &&
		typclass->typtype == 'b' &&
		att->atttypid < FirstNormalObjectId &&
		typclass->typelem == InvalidOid)
	{
		return 'b';
	}
	/*
	 * Use send/recv, if allowed, if the type is plain or builtin.
	 *
	 * XXX: we can't use send/recv for array or composite types for now due to
	 * the embedded oids.
	 */
	else if (allow_binary_basetypes &&
			 OidIsValid(typclass->typreceive) &&
			 (att->atttypid < FirstNormalObjectId || typclass->typtype != 'c') &&
			 (att->atttypid < FirstNormalObjectId || typclass->typelem == InvalidOid))
	{
		return 's';
	}

	return 't';
}


PGLogicalProtoAPI *
pglogical_init_api(PGLogicalProtoType typ)
{
    PGLogicalProtoAPI* res = palloc0(sizeof(PGLogicalProtoAPI));
	sscanf(MyReplicationSlot->data.name.data, MULTIMASTER_SLOT_PATTERN, &MtmReplicationNodeId);
	MTM_LOG1("%d: PRGLOGICAL init API for slot %s node %d", MyProcPid, MyReplicationSlot->data.name.data, MtmReplicationNodeId);
    res->write_rel = pglogical_write_rel;
    res->write_begin = pglogical_write_begin;
	res->write_message = pglogical_write_message;
    res->write_commit = pglogical_write_commit;
    res->write_insert = pglogical_write_insert;
    res->write_update = pglogical_write_update;
    res->write_delete = pglogical_write_delete;
	res->setup_hooks = MtmSetupReplicationHooks;
    res->write_startup_message = write_startup_message;
    return res;
}

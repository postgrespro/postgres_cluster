/*-------------------------------------------------------------------------
 *
 * test_decoding.c
 *		  example logical decoding output plugin
 *
 * Copyright (c) 2012-2017, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  contrib/test_decoding/test_decoding.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/sysattr.h"

#include "catalog/pg_class.h"
#include "catalog/pg_type.h"

#include "nodes/parsenodes.h"

#include "replication/output_plugin.h"
#include "replication/logical.h"
#include "replication/message.h"
#include "replication/origin.h"

#include "storage/procarray.h"

#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

#include "access/xact.h"
#include "miscadmin.h"
#include "executor/executor.h"
#include "nodes/nodes.h"
#include "postmaster/autovacuum.h"
#include "replication/walsender.h"
#include "storage/latch.h"
#include "storage/proc.h"
#include "storage/ipc.h"
#include "pgstat.h"
#include "tcop/utility.h"
#include "commands/portalcmds.h"

PG_MODULE_MAGIC;

/* These must be available to pg_dlsym() */
extern void _PG_init(void);
extern void _PG_output_plugin_init(OutputPluginCallbacks *cb);

typedef struct
{
	MemoryContext context;
	bool		include_xids;
	bool		include_timestamp;
	bool		skip_empty_xacts;
	bool		xact_wrote_changes;
	bool		only_local;
	bool		twophase_decoding;
	bool		twophase_decode_with_catalog_changes;
} TestDecodingData;

static void pg_decode_startup(LogicalDecodingContext *ctx, OutputPluginOptions *opt,
				  bool is_init);
static void pg_decode_shutdown(LogicalDecodingContext *ctx);
static void pg_decode_begin_txn(LogicalDecodingContext *ctx,
					ReorderBufferTXN *txn);
static void pg_output_begin(LogicalDecodingContext *ctx,
				TestDecodingData *data,
				ReorderBufferTXN *txn,
				bool last_write);
static void pg_decode_commit_txn(LogicalDecodingContext *ctx,
					 ReorderBufferTXN *txn, XLogRecPtr commit_lsn);
static void pg_decode_change(LogicalDecodingContext *ctx,
				 ReorderBufferTXN *txn, Relation rel,
				 ReorderBufferChange *change);
static bool pg_decode_filter(LogicalDecodingContext *ctx,
				 RepOriginId origin_id);
static void pg_decode_message(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn, XLogRecPtr message_lsn,
				  bool transactional, const char *prefix,
				  Size sz, const char *message);
static bool pg_filter_prepare(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn,
				  char *gid);
static void pg_decode_prepare_txn(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn,
				  XLogRecPtr prepare_lsn);
static void pg_decode_commit_prepared_txn(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn,
				  XLogRecPtr commit_lsn);
static void pg_decode_abort_prepared_txn(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn,
				  XLogRecPtr abort_lsn);

static void test_decoding_xact_callback(XactEvent event, void *arg);

static void test_decoding_process_utility(PlannedStmt *pstmt,
					const char *queryString, ProcessUtilityContext context,
					ParamListInfo params, QueryEnvironment *queryEnv,
					DestReceiver *dest, char *completionTag);

static bool test_decoding_twophase_commit();

static void test_decoding_executor_finish(QueryDesc *queryDesc);

static ProcessUtility_hook_type PreviousProcessUtilityHook;

static ExecutorFinish_hook_type PreviousExecutorFinishHook;

static bool CurrentTxContainsDML;
static bool CurrentTxContainsDDL;
static bool CurrentTxNonpreparable;

void
_PG_init(void)
{
	PreviousExecutorFinishHook = ExecutorFinish_hook;
	ExecutorFinish_hook = test_decoding_executor_finish;

	PreviousProcessUtilityHook = ProcessUtility_hook;
	ProcessUtility_hook = test_decoding_process_utility;

	if (!IsUnderPostmaster)
		RegisterXactCallback(test_decoding_xact_callback, NULL);
}


/* ability to hook into sigle-statement transaction */
static void
test_decoding_xact_callback(XactEvent event, void *arg)
{
	switch (event)
	{
		case XACT_EVENT_START:
		case XACT_EVENT_ABORT:
			CurrentTxContainsDML = false;
			CurrentTxContainsDDL = false;
			CurrentTxNonpreparable = false;
			break;
		case XACT_EVENT_COMMIT_COMMAND:
			if (!IsTransactionBlock())
				test_decoding_twophase_commit();
			break;
		default:
			break;
	}
}

/* find out whether transaction had wrote any data or not */
static void
test_decoding_executor_finish(QueryDesc *queryDesc)
{
	CmdType operation = queryDesc->operation;
	EState *estate = queryDesc->estate;
	if (estate->es_processed != 0 &&
		(operation == CMD_INSERT || operation == CMD_UPDATE || operation == CMD_DELETE))
	{
		int i;
		for (i = 0; i < estate->es_num_result_relations; i++)
		{
			Relation rel = estate->es_result_relations[i].ri_RelationDesc;
			if (RelationNeedsWAL(rel)) {
				CurrentTxContainsDML = true;
				break;
			}
		}
	}

	if (PreviousExecutorFinishHook != NULL)
		PreviousExecutorFinishHook(queryDesc);
	else
		standard_ExecutorFinish(queryDesc);
}


/*
 * Several things here:
 * 1) hook into commit of transaction block
 * 2) write logical message for DDL (default path)
 * 3) prevent 2pc hook for tx that can not be prepared and
 *    send them as logical nontransactional message.
 */
static void
test_decoding_process_utility(PlannedStmt *pstmt,
					const char *queryString, ProcessUtilityContext context,
					ParamListInfo params, QueryEnvironment *queryEnv,
					DestReceiver *dest, char *completionTag)
{
	Node	   *parsetree = pstmt->utilityStmt;

	switch (nodeTag(parsetree))
	{
		case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;
				switch (stmt->kind)
				{
					case TRANS_STMT_COMMIT:
						if (test_decoding_twophase_commit())
							return; /* do not proceed */
						break;
					default:
						break;
				}
			}
			break;

		/* cannot PREPARE a transaction that has executed LISTEN, UNLISTEN, or NOTIFY */
		case T_NotifyStmt:
		case T_ListenStmt:
		case T_UnlistenStmt:
			CurrentTxNonpreparable = true;
			break;

		/* create/reindex/drop concurrently can not be execuled in prepared tx */
		case T_ReindexStmt:
			{
				ReindexStmt *stmt = (ReindexStmt *) parsetree;
				switch (stmt->kind)
				{
					case REINDEX_OBJECT_SCHEMA:
					case REINDEX_OBJECT_SYSTEM:
					case REINDEX_OBJECT_DATABASE:
						CurrentTxNonpreparable = true;
					default:
						break;
				}
			}
			break;
		case T_IndexStmt:
			{
				IndexStmt *indexStmt = (IndexStmt *) parsetree;
				if (indexStmt->concurrent)
					CurrentTxNonpreparable = true;
			}
			break;
		case T_DropStmt:
			{
				DropStmt *stmt = (DropStmt *) parsetree;
				if (stmt->removeType == OBJECT_INDEX && stmt->concurrent)
					CurrentTxNonpreparable = true;
			}
			break;

		/* cannot PREPARE a transaction that has created a cursor WITH HOLD */
		case T_DeclareCursorStmt:
			{
				DeclareCursorStmt *stmt = (DeclareCursorStmt *) parsetree;
				if (stmt->options & CURSOR_OPT_HOLD)
					CurrentTxNonpreparable = true;
			}
			break;

		default:
			LogLogicalMessage("D", queryString, strlen(queryString) + 1, true);
			CurrentTxContainsDDL = true;
			break;
	}

	/* Send non-transactional message then */
	if (CurrentTxNonpreparable)
		LogLogicalMessage("C", queryString, strlen(queryString) + 1, false);

	if (PreviousProcessUtilityHook != NULL)
	{
		PreviousProcessUtilityHook(pstmt, queryString, context, params, queryEnv,
								   dest, completionTag);
	}
	else
	{
		standard_ProcessUtility(pstmt, queryString, context, params, queryEnv,
								dest, completionTag);
	}
}

/*
 * Change commit to prepare and wait on latch.
 * WalSender will unlock us after decoding and we can proceed.
 */
static bool
test_decoding_twophase_commit()
{
	int result = 0;
	char gid[20];

	if (IsAutoVacuumLauncherProcess() ||
			!IsNormalProcessingMode() ||
			am_walsender ||
			IsBackgroundWorker ||
			IsAutoVacuumWorkerProcess() ||
			IsAbortedTransactionBlockState() ||
			!(CurrentTxContainsDML || CurrentTxContainsDDL) ||
			CurrentTxNonpreparable )
		return false;

	snprintf(gid, sizeof(gid), "test_decoding:%d", MyProc->pgprocno);

	if (!IsTransactionBlock())
	{
		BeginTransactionBlock();
		CommitTransactionCommand();
		StartTransactionCommand();
	}
	if (!PrepareTransactionBlock(gid))
	{
		fprintf(stderr, "Can't prepare transaction '%s'\n", gid);
	}
	CommitTransactionCommand();

	result = WaitLatch(&MyProc->procLatch, WL_LATCH_SET | WL_POSTMASTER_DEATH, 0,
													WAIT_EVENT_REPLICATION_SLOT_SYNC);

	if (result & WL_POSTMASTER_DEATH)
		proc_exit(1);

	if (result & WL_LATCH_SET)
		ResetLatch(&MyProc->procLatch);


	StartTransactionCommand();
	FinishPreparedTransaction(gid, true);
	return true;
}

/* specify output plugin callbacks */
void
_PG_output_plugin_init(OutputPluginCallbacks *cb)
{
	AssertVariableIsOfType(&_PG_output_plugin_init, LogicalOutputPluginInit);

	cb->startup_cb = pg_decode_startup;
	cb->begin_cb = pg_decode_begin_txn;
	cb->change_cb = pg_decode_change;
	cb->commit_cb = pg_decode_commit_txn;

	cb->filter_by_origin_cb = pg_decode_filter;
	cb->shutdown_cb = pg_decode_shutdown;
	cb->message_cb = pg_decode_message;

	cb->filter_prepare_cb = pg_filter_prepare;
	cb->prepare_cb = pg_decode_prepare_txn;
	cb->commit_prepared_cb = pg_decode_commit_prepared_txn;
	cb->abort_prepared_cb = pg_decode_abort_prepared_txn;
}


/* initialize this plugin */
static void
pg_decode_startup(LogicalDecodingContext *ctx, OutputPluginOptions *opt,
				  bool is_init)
{
	ListCell   *option;
	TestDecodingData *data;

	data = palloc0(sizeof(TestDecodingData));
	data->context = AllocSetContextCreate(ctx->context,
										  "text conversion context",
										  ALLOCSET_DEFAULT_SIZES);
	data->include_xids = true;
	data->include_timestamp = false;
	data->skip_empty_xacts = false;
	data->only_local = false;
	data->twophase_decoding = false;
	data->twophase_decode_with_catalog_changes = false;

	ctx->output_plugin_private = data;

	opt->output_type = OUTPUT_PLUGIN_TEXTUAL_OUTPUT;

	foreach(option, ctx->output_plugin_options)
	{
		DefElem    *elem = lfirst(option);

		Assert(elem->arg == NULL || IsA(elem->arg, String));

		if (strcmp(elem->defname, "include-xids") == 0)
		{
			/* if option does not provide a value, it means its value is true */
			if (elem->arg == NULL)
				data->include_xids = true;
			else if (!parse_bool(strVal(elem->arg), &data->include_xids))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else if (strcmp(elem->defname, "include-timestamp") == 0)
		{
			if (elem->arg == NULL)
				data->include_timestamp = true;
			else if (!parse_bool(strVal(elem->arg), &data->include_timestamp))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else if (strcmp(elem->defname, "force-binary") == 0)
		{
			bool		force_binary;

			if (elem->arg == NULL)
				continue;
			else if (!parse_bool(strVal(elem->arg), &force_binary))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));

			if (force_binary)
				opt->output_type = OUTPUT_PLUGIN_BINARY_OUTPUT;
		}
		else if (strcmp(elem->defname, "skip-empty-xacts") == 0)
		{

			if (elem->arg == NULL)
				data->skip_empty_xacts = true;
			else if (!parse_bool(strVal(elem->arg), &data->skip_empty_xacts))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else if (strcmp(elem->defname, "only-local") == 0)
		{

			if (elem->arg == NULL)
				data->only_local = true;
			else if (!parse_bool(strVal(elem->arg), &data->only_local))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else if (strcmp(elem->defname, "twophase-decoding") == 0)
		{

			if (elem->arg == NULL)
				data->twophase_decoding = true;
			else if (!parse_bool(strVal(elem->arg), &data->twophase_decoding))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else if (strcmp(elem->defname, "twophase-decode-with-catalog-changes") == 0)
		{
			if (elem->arg == NULL)
				data->twophase_decode_with_catalog_changes = true;
			else if (!parse_bool(strVal(elem->arg), &data->twophase_decode_with_catalog_changes))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("could not parse value \"%s\" for parameter \"%s\"",
						 strVal(elem->arg), elem->defname)));
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("option \"%s\" = \"%s\" is unknown",
							elem->defname,
							elem->arg ? strVal(elem->arg) : "(null)")));
		}
	}
}

/* cleanup this plugin's resources */
static void
pg_decode_shutdown(LogicalDecodingContext *ctx)
{
	TestDecodingData *data = ctx->output_plugin_private;

	/* cleanup our own resources via memory context reset */
	MemoryContextDelete(data->context);
}

/* BEGIN callback */
static void
pg_decode_begin_txn(LogicalDecodingContext *ctx, ReorderBufferTXN *txn)
{
	TestDecodingData *data = ctx->output_plugin_private;

	data->xact_wrote_changes = false;
	if (data->skip_empty_xacts)
		return;

	pg_output_begin(ctx, data, txn, true);
}

static void
pg_output_begin(LogicalDecodingContext *ctx, TestDecodingData *data, ReorderBufferTXN *txn, bool last_write)
{
	OutputPluginPrepareWrite(ctx, last_write);
	if (data->include_xids)
		appendStringInfo(ctx->out, "BEGIN %u", txn->xid);
	else
		appendStringInfoString(ctx->out, "BEGIN");
	OutputPluginWrite(ctx, last_write);
}

/* COMMIT callback */
static void
pg_decode_commit_txn(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					 XLogRecPtr commit_lsn)
{
	TestDecodingData *data = ctx->output_plugin_private;

	if (data->skip_empty_xacts && !data->xact_wrote_changes)
		return;

	OutputPluginPrepareWrite(ctx, true);

	appendStringInfoString(ctx->out, "COMMIT");

	if (data->include_xids)
		appendStringInfo(ctx->out, " %u", txn->xid);

	if (data->include_timestamp)
		appendStringInfo(ctx->out, " (at %s)",
						 timestamptz_to_str(txn->commit_time));

	OutputPluginWrite(ctx, true);
}


/* Filter out unnecessary two-phase transactions */
static bool
pg_filter_prepare(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					char *gid)
{
	/* decode only tx that are prepared by our hook */
	if (strncmp(gid, "test_decoding:", 14) == 0)
		return false;
	else
		return true;
}


/* PREPARE callback */
static void
pg_decode_prepare_txn(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					XLogRecPtr prepare_lsn)
{
	TestDecodingData *data = ctx->output_plugin_private;
	int		backend_procno;

	// if (data->skip_empty_xacts && !data->xact_wrote_changes)
	// 	return;

	OutputPluginPrepareWrite(ctx, true);

	appendStringInfo(ctx->out, "PREPARE TRANSACTION %s",
		quote_literal_cstr(txn->gid));

	if (data->include_xids)
		appendStringInfo(ctx->out, " %u", txn->xid);

	if (data->include_timestamp)
		appendStringInfo(ctx->out, " (at %s)",
						 timestamptz_to_str(txn->commit_time));

	OutputPluginWrite(ctx, true);

	/* Unlock backend */
	sscanf(txn->gid, "test_decoding:%d", &backend_procno);
	SetLatch(&ProcGlobal->allProcs[backend_procno].procLatch);
}

/* COMMIT PREPARED callback */
static void
pg_decode_commit_prepared_txn(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					XLogRecPtr commit_lsn)
{
	TestDecodingData *data = ctx->output_plugin_private;

	// if (data->skip_empty_xacts && !data->xact_wrote_changes)
	// 	return;

	OutputPluginPrepareWrite(ctx, true);

	appendStringInfo(ctx->out, "COMMIT PREPARED %s",
		quote_literal_cstr(txn->gid));

	if (data->include_xids)
		appendStringInfo(ctx->out, " %u", txn->xid);

	if (data->include_timestamp)
		appendStringInfo(ctx->out, " (at %s)",
						 timestamptz_to_str(txn->commit_time));

	OutputPluginWrite(ctx, true);
}

/* ABORT PREPARED callback */
static void
pg_decode_abort_prepared_txn(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					XLogRecPtr abort_lsn)
{
	TestDecodingData *data = ctx->output_plugin_private;

	// if (data->skip_empty_xacts && !data->xact_wrote_changes)
	// 	return;

	OutputPluginPrepareWrite(ctx, true);

	appendStringInfo(ctx->out, "ABORT PREPARED %s",
		quote_literal_cstr(txn->gid));

	if (data->include_xids)
		appendStringInfo(ctx->out, " %u", txn->xid);

	if (data->include_timestamp)
		appendStringInfo(ctx->out, " (at %s)",
						 timestamptz_to_str(txn->commit_time));

	OutputPluginWrite(ctx, true);
}

static bool
pg_decode_filter(LogicalDecodingContext *ctx,
				 RepOriginId origin_id)
{
	TestDecodingData *data = ctx->output_plugin_private;

	if (data->only_local && origin_id != InvalidRepOriginId)
		return true;
	return false;
}

/*
 * Print literal `outputstr' already represented as string of type `typid'
 * into stringbuf `s'.
 *
 * Some builtin types aren't quoted, the rest is quoted. Escaping is done as
 * if standard_conforming_strings were enabled.
 */
static void
print_literal(StringInfo s, Oid typid, char *outputstr)
{
	const char *valptr;

	switch (typid)
	{
		case INT2OID:
		case INT4OID:
		case INT8OID:
		case OIDOID:
		case FLOAT4OID:
		case FLOAT8OID:
		case NUMERICOID:
			/* NB: We don't care about Inf, NaN et al. */
			appendStringInfoString(s, outputstr);
			break;

		case BITOID:
		case VARBITOID:
			appendStringInfo(s, "B'%s'", outputstr);
			break;

		case BOOLOID:
			if (strcmp(outputstr, "t") == 0)
				appendStringInfoString(s, "true");
			else
				appendStringInfoString(s, "false");
			break;

		default:
			appendStringInfoChar(s, '\'');
			for (valptr = outputstr; *valptr; valptr++)
			{
				char		ch = *valptr;

				if (SQL_STR_DOUBLE(ch, false))
					appendStringInfoChar(s, ch);
				appendStringInfoChar(s, ch);
			}
			appendStringInfoChar(s, '\'');
			break;
	}
}

/* print the tuple 'tuple' into the StringInfo s */
static void
tuple_to_stringinfo(StringInfo s, TupleDesc tupdesc, HeapTuple tuple, bool skip_nulls)
{
	int			natt;
	Oid			oid;

	/* print oid of tuple, it's not included in the TupleDesc */
	if ((oid = HeapTupleHeaderGetOid(tuple->t_data)) != InvalidOid)
	{
		appendStringInfo(s, " oid[oid]:%u", oid);
	}

	/* print all columns individually */
	for (natt = 0; natt < tupdesc->natts; natt++)
	{
		Form_pg_attribute attr; /* the attribute itself */
		Oid			typid;		/* type of current attribute */
		Oid			typoutput;	/* output function */
		bool		typisvarlena;
		Datum		origval;	/* possibly toasted Datum */
		bool		isnull;		/* column is null? */

		attr = tupdesc->attrs[natt];

		/*
		 * don't print dropped columns, we can't be sure everything is
		 * available for them
		 */
		if (attr->attisdropped)
			continue;

		/*
		 * Don't print system columns, oid will already have been printed if
		 * present.
		 */
		if (attr->attnum < 0)
			continue;

		typid = attr->atttypid;

		/* get Datum from tuple */
		origval = heap_getattr(tuple, natt + 1, tupdesc, &isnull);

		if (isnull && skip_nulls)
			continue;

		/* print attribute name */
		appendStringInfoChar(s, ' ');
		appendStringInfoString(s, quote_identifier(NameStr(attr->attname)));

		/* print attribute type */
		appendStringInfoChar(s, '[');
		appendStringInfoString(s, format_type_be(typid));
		appendStringInfoChar(s, ']');

		/* query output function */
		getTypeOutputInfo(typid,
						  &typoutput, &typisvarlena);

		/* print separator */
		appendStringInfoChar(s, ':');

		/* print data */
		if (isnull)
			appendStringInfoString(s, "null");
		else if (typisvarlena && VARATT_IS_EXTERNAL_ONDISK(origval))
			appendStringInfoString(s, "unchanged-toast-datum");
		else if (!typisvarlena)
			print_literal(s, typid,
						  OidOutputFunctionCall(typoutput, origval));
		else
		{
			Datum		val;	/* definitely detoasted Datum */

			val = PointerGetDatum(PG_DETOAST_DATUM(origval));
			print_literal(s, typid, OidOutputFunctionCall(typoutput, val));
		}
	}
}

/*
 * callback for individual changed tuples
 */
static void
pg_decode_change(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
				 Relation relation, ReorderBufferChange *change)
{
	TestDecodingData *data;
	Form_pg_class class_form;
	TupleDesc	tupdesc;
	MemoryContext old;

	data = ctx->output_plugin_private;

	/* output BEGIN if we haven't yet */
	if (data->skip_empty_xacts && !data->xact_wrote_changes)
	{
		pg_output_begin(ctx, data, txn, false);
	}
	data->xact_wrote_changes = true;

	class_form = RelationGetForm(relation);
	tupdesc = RelationGetDescr(relation);

	/* Avoid leaking memory by using and resetting our own context */
	old = MemoryContextSwitchTo(data->context);

	OutputPluginPrepareWrite(ctx, true);

	appendStringInfoString(ctx->out, "table ");
	appendStringInfoString(ctx->out,
						   quote_qualified_identifier(
													  get_namespace_name(
							  get_rel_namespace(RelationGetRelid(relation))),
											  NameStr(class_form->relname)));
	appendStringInfoChar(ctx->out, ':');

	switch (change->action)
	{
		case REORDER_BUFFER_CHANGE_INSERT:
			appendStringInfoString(ctx->out, " INSERT:");
			if (change->data.tp.newtuple == NULL)
				appendStringInfoString(ctx->out, " (no-tuple-data)");
			else
				tuple_to_stringinfo(ctx->out, tupdesc,
									&change->data.tp.newtuple->tuple,
									false);
			break;
		case REORDER_BUFFER_CHANGE_UPDATE:
			appendStringInfoString(ctx->out, " UPDATE:");
			if (change->data.tp.oldtuple != NULL)
			{
				appendStringInfoString(ctx->out, " old-key:");
				tuple_to_stringinfo(ctx->out, tupdesc,
									&change->data.tp.oldtuple->tuple,
									true);
				appendStringInfoString(ctx->out, " new-tuple:");
			}

			if (change->data.tp.newtuple == NULL)
				appendStringInfoString(ctx->out, " (no-tuple-data)");
			else
				tuple_to_stringinfo(ctx->out, tupdesc,
									&change->data.tp.newtuple->tuple,
									false);
			break;
		case REORDER_BUFFER_CHANGE_DELETE:
			appendStringInfoString(ctx->out, " DELETE:");

			/* if there was no PK, we only know that a delete happened */
			if (change->data.tp.oldtuple == NULL)
				appendStringInfoString(ctx->out, " (no-tuple-data)");
			/* In DELETE, only the replica identity is present; display that */
			else
				tuple_to_stringinfo(ctx->out, tupdesc,
									&change->data.tp.oldtuple->tuple,
									true);
			break;
		default:
			Assert(false);
	}

	MemoryContextSwitchTo(old);
	MemoryContextReset(data->context);

	OutputPluginWrite(ctx, true);
}

static void
pg_decode_message(LogicalDecodingContext *ctx,
				  ReorderBufferTXN *txn, XLogRecPtr lsn, bool transactional,
				  const char *prefix, Size sz, const char *message)
{
	OutputPluginPrepareWrite(ctx, true);
	appendStringInfo(ctx->out, "message: transactional: %d prefix: %s, sz: %zu content:",
					 transactional, prefix, sz);
	appendBinaryStringInfo(ctx->out, message, sz);
	OutputPluginWrite(ctx, true);
}

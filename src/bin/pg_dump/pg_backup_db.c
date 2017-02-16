/*-------------------------------------------------------------------------
 *
 * pg_backup_db.c
 *
 *	Implements the basic DB functions used by the archiver.
 *
 * IDENTIFICATION
 *	  src/bin/pg_dump/pg_backup_db.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include "dumputils.h"
#include "fe_utils/string_utils.h"
#include "parallel.h"
#include "pg_backup_archiver.h"
#include "pg_backup_db.h"
#include "pg_backup_utils.h"

#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif


#define DB_MAX_ERR_STMT 128

/* translator: this is a module name */
static const char *modulename = gettext_noop("archiver (db)");

static void _check_database_version(ArchiveHandle *AH);
static PGconn *_connectDB(ArchiveHandle *AH, const char *newdbname, const char *newUser);
static void notice_processor(void *arg, const char *message);

static void
_check_database_version(ArchiveHandle *AH)
{
	const char *remoteversion_str;
	int			remoteversion;
	PGresult   *res;

	remoteversion_str = PQparameterStatus(AH->connection, "server_version");
	remoteversion = PQserverVersion(AH->connection);
	if (remoteversion == 0 || !remoteversion_str)
		exit_horribly(modulename, "could not get server_version from libpq\n");

	AH->public.remoteVersionStr = pg_strdup(remoteversion_str);
	AH->public.remoteVersion = remoteversion;
	if (!AH->archiveRemoteVersion)
		AH->archiveRemoteVersion = AH->public.remoteVersionStr;

	if (remoteversion != PG_VERSION_NUM
		&& (remoteversion < AH->public.minRemoteVersion ||
			remoteversion > AH->public.maxRemoteVersion))
	{
		write_msg(NULL, "server version: %s; %s version: %s\n",
				  remoteversion_str, progname, PG_VERSION);
		exit_horribly(NULL, "aborting because of server version mismatch\n");
	}

	/*
	 * When running against 9.0 or later, check if we are in recovery mode,
	 * which means we are on a hot standby.
	 */
	if (remoteversion >= 90000)
	{
		res = ExecuteSqlQueryForSingleRow((Archive *) AH, "SELECT pg_catalog.pg_is_in_recovery()");

		AH->public.isStandby = (strcmp(PQgetvalue(res, 0, 0), "t") == 0);
		PQclear(res);
	}
	else
		AH->public.isStandby = false;
}

/*
 * Reconnect to the server.  If dbname is not NULL, use that database,
 * else the one associated with the archive handle.  If username is
 * not NULL, use that user name, else the one from the handle.  If
 * both the database and the user match the existing connection already,
 * nothing will be done.
 *
 * Returns 1 in any case.
 */
int
ReconnectToServer(ArchiveHandle *AH, const char *dbname, const char *username)
{
	PGconn	   *newConn;
	const char *newdbname;
	const char *newusername;

	if (!dbname)
		newdbname = PQdb(AH->connection);
	else
		newdbname = dbname;

	if (!username)
		newusername = PQuser(AH->connection);
	else
		newusername = username;

	/* Let's see if the request is already satisfied */
	if (strcmp(newdbname, PQdb(AH->connection)) == 0 &&
		strcmp(newusername, PQuser(AH->connection)) == 0)
		return 1;

	newConn = _connectDB(AH, newdbname, newusername);

	/* Update ArchiveHandle's connCancel before closing old connection */
	set_archive_cancel_info(AH, newConn);

	PQfinish(AH->connection);
	AH->connection = newConn;

	return 1;
}

/*
 * Connect to the db again.
 *
 * Note: it's not really all that sensible to use a single-entry password
 * cache if the username keeps changing.  In current usage, however, the
 * username never does change, so one savedPassword is sufficient.  We do
 * update the cache on the off chance that the password has changed since the
 * start of the run.
 */
static PGconn *
_connectDB(ArchiveHandle *AH, const char *reqdb, const char *requser)
{
	PQExpBufferData connstr;
	PGconn	   *newConn;
	const char *newdb;
	const char *newuser;
	char	   *password;
	bool		new_pass;

	if (!reqdb)
		newdb = PQdb(AH->connection);
	else
		newdb = reqdb;

	if (!requser || strlen(requser) == 0)
		newuser = PQuser(AH->connection);
	else
		newuser = requser;

	ahlog(AH, 1, "connecting to database \"%s\" as user \"%s\"\n",
		  newdb, newuser);

	password = AH->savedPassword ? pg_strdup(AH->savedPassword) : NULL;

	if (AH->promptPassword == TRI_YES && password == NULL)
	{
		password = simple_prompt("Password: ", 100, false);
		if (password == NULL)
			exit_horribly(modulename, "out of memory\n");
	}

	initPQExpBuffer(&connstr);
	appendPQExpBuffer(&connstr, "dbname=");
	appendConnStrVal(&connstr, newdb);

	do
	{
		const char *keywords[7];
		const char *values[7];

		keywords[0] = "host";
		values[0] = PQhost(AH->connection);
		keywords[1] = "port";
		values[1] = PQport(AH->connection);
		keywords[2] = "user";
		values[2] = newuser;
		keywords[3] = "password";
		values[3] = password;
		keywords[4] = "dbname";
		values[4] = connstr.data;
		keywords[5] = "fallback_application_name";
		values[5] = progname;
		keywords[6] = NULL;
		values[6] = NULL;

		new_pass = false;
		newConn = PQconnectdbParams(keywords, values, true);

		if (!newConn)
			exit_horribly(modulename, "failed to reconnect to database\n");

		if (PQstatus(newConn) == CONNECTION_BAD)
		{
			if (!PQconnectionNeedsPassword(newConn))
				exit_horribly(modulename, "could not reconnect to database: %s",
							  PQerrorMessage(newConn));
			PQfinish(newConn);

			if (password)
				fprintf(stderr, "Password incorrect\n");

			fprintf(stderr, "Connecting to %s as %s\n",
					newdb, newuser);

			if (password)
				free(password);

			if (AH->promptPassword != TRI_NO)
				password = simple_prompt("Password: ", 100, false);
			else
				exit_horribly(modulename, "connection needs password\n");

			if (password == NULL)
				exit_horribly(modulename, "out of memory\n");
			new_pass = true;
		}
	} while (new_pass);

	/*
	 * We want to remember connection's actual password, whether or not we got
	 * it by prompting.  So we don't just store the password variable.
	 */
	if (PQconnectionUsedPassword(newConn))
	{
		if (AH->savedPassword)
			free(AH->savedPassword);
		AH->savedPassword = pg_strdup(PQpass(newConn));
	}
	if (password)
		free(password);

	termPQExpBuffer(&connstr);

	/* check for version mismatch */
	_check_database_version(AH);

	PQsetNoticeProcessor(newConn, notice_processor, NULL);

	return newConn;
}


/*
 * Make a database connection with the given parameters.  The
 * connection handle is returned, the parameters are stored in AHX.
 * An interactive password prompt is automatically issued if required.
 *
 * Note: it's not really all that sensible to use a single-entry password
 * cache if the username keeps changing.  In current usage, however, the
 * username never does change, so one savedPassword is sufficient.
 */
void
ConnectDatabase(Archive *AHX,
				const char *dbname,
				const char *pghost,
				const char *pgport,
				const char *username,
				trivalue prompt_password)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	char	   *password;
	bool		new_pass;

	if (AH->connection)
		exit_horribly(modulename, "already connected to a database\n");

	password = AH->savedPassword ? pg_strdup(AH->savedPassword) : NULL;

	if (prompt_password == TRI_YES && password == NULL)
	{
		password = simple_prompt("Password: ", 100, false);
		if (password == NULL)
			exit_horribly(modulename, "out of memory\n");
	}
	AH->promptPassword = prompt_password;

	/*
	 * Start the connection.  Loop until we have a password if requested by
	 * backend.
	 */
	do
	{
		const char *keywords[7];
		const char *values[7];

		keywords[0] = "host";
		values[0] = pghost;
		keywords[1] = "port";
		values[1] = pgport;
		keywords[2] = "user";
		values[2] = username;
		keywords[3] = "password";
		values[3] = password;
		keywords[4] = "dbname";
		values[4] = dbname;
		keywords[5] = "fallback_application_name";
		values[5] = progname;
		keywords[6] = NULL;
		values[6] = NULL;

		new_pass = false;
		AH->connection = PQconnectdbParams(keywords, values, true);

		if (!AH->connection)
			exit_horribly(modulename, "failed to connect to database\n");

		if (PQstatus(AH->connection) == CONNECTION_BAD &&
			PQconnectionNeedsPassword(AH->connection) &&
			password == NULL &&
			prompt_password != TRI_NO)
		{
			PQfinish(AH->connection);
			password = simple_prompt("Password: ", 100, false);
			if (password == NULL)
				exit_horribly(modulename, "out of memory\n");
			new_pass = true;
		}
	} while (new_pass);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(AH->connection) == CONNECTION_BAD)
		exit_horribly(modulename, "connection to database \"%s\" failed: %s",
					  PQdb(AH->connection) ? PQdb(AH->connection) : "",
					  PQerrorMessage(AH->connection));

	/*
	 * We want to remember connection's actual password, whether or not we got
	 * it by prompting.  So we don't just store the password variable.
	 */
	if (PQconnectionUsedPassword(AH->connection))
	{
		if (AH->savedPassword)
			free(AH->savedPassword);
		AH->savedPassword = pg_strdup(PQpass(AH->connection));
	}
	if (password)
		free(password);

	/* check for version mismatch */
	_check_database_version(AH);

	PQsetNoticeProcessor(AH->connection, notice_processor, NULL);

	/* arrange for SIGINT to issue a query cancel on this connection */
	set_archive_cancel_info(AH, AH->connection);
}

/*
 * Close the connection to the database and also cancel off the query if we
 * have one running.
 */
void
DisconnectDatabase(Archive *AHX)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	char		errbuf[1];

	if (!AH->connection)
		return;

	if (AH->connCancel)
	{
		/*
		 * If we have an active query, send a cancel before closing.  This is
		 * of no use for a normal exit, but might be helpful during
		 * exit_horribly().
		 */
		if (PQtransactionStatus(AH->connection) == PQTRANS_ACTIVE)
			PQcancel(AH->connCancel, errbuf, sizeof(errbuf));

		/*
		 * Prevent signal handler from sending a cancel after this.
		 */
		set_archive_cancel_info(AH, NULL);
	}

	PQfinish(AH->connection);
	AH->connection = NULL;
}

PGconn *
GetConnection(Archive *AHX)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;

	return AH->connection;
}

static void
notice_processor(void *arg, const char *message)
{
	write_msg(NULL, "%s", message);
}

/* Like exit_horribly(), but with a complaint about a particular query. */
static void
die_on_query_failure(ArchiveHandle *AH, const char *modulename, const char *query)
{
	write_msg(modulename, "query failed: %s",
			  PQerrorMessage(AH->connection));
	exit_horribly(modulename, "query was: %s\n", query);
}

void
ExecuteSqlStatement(Archive *AHX, const char *query)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	PGresult   *res;

	res = PQexec(AH->connection, query);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
		die_on_query_failure(AH, modulename, query);
	PQclear(res);
}

PGresult *
ExecuteSqlQuery(Archive *AHX, const char *query, ExecStatusType status)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	PGresult   *res;

	res = PQexec(AH->connection, query);
	if (PQresultStatus(res) != status)
		die_on_query_failure(AH, modulename, query);
	return res;
}

/*
 * Execute an SQL query and verify that we got exactly one row back.
 */
PGresult *
ExecuteSqlQueryForSingleRow(Archive *fout, char *query)
{
	PGresult   *res;
	int			ntups;

	res = ExecuteSqlQuery(fout, query, PGRES_TUPLES_OK);

	/* Expecting a single result only */
	ntups = PQntuples(res);
	if (ntups != 1)
		exit_horribly(NULL,
					  ngettext("query returned %d row instead of one: %s\n",
							   "query returned %d rows instead of one: %s\n",
							   ntups),
					  ntups, query);

	return res;
}

PGresult *
ExecuteSqlQueryBin(Archive *AHX, const char *command, int nParams,
				   const Oid *paramTypes, const char * const *paramValues,
				   const int *paramLengths, const int *paramFormats,
				   int resultFormat, ExecStatusType status)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	PGresult   *res;

	res = PQexecParams(AH->connection, command,
					   nParams, paramTypes,
					   paramValues, paramLengths,
					   paramFormats, resultFormat);
	if (PQresultStatus(res) != status)
		die_on_query_failure(AH, modulename, command);
	return res;
}

/*
 * Convenience function to send a query.
 * Monitors result to detect COPY statements
 */
static void
ExecuteSqlCommand(ArchiveHandle *AH, const char *qry, const char *desc)
{
	PGconn	   *conn = AH->connection;
	PGresult   *res;
	char		errStmt[DB_MAX_ERR_STMT];

#ifdef NOT_USED
	fprintf(stderr, "Executing: '%s'\n\n", qry);
#endif
	res = PQexec(conn, qry);

	switch (PQresultStatus(res))
	{
		case PGRES_COMMAND_OK:
		case PGRES_TUPLES_OK:
		case PGRES_EMPTY_QUERY:
			/* A-OK */
			break;
		case PGRES_COPY_IN:
			/* Assume this is an expected result */
			AH->pgCopyIn = true;
			break;
		default:
			/* trouble */
			strncpy(errStmt, qry, DB_MAX_ERR_STMT);		/* strncpy required here */
			if (errStmt[DB_MAX_ERR_STMT - 1] != '\0')
			{
				errStmt[DB_MAX_ERR_STMT - 4] = '.';
				errStmt[DB_MAX_ERR_STMT - 3] = '.';
				errStmt[DB_MAX_ERR_STMT - 2] = '.';
				errStmt[DB_MAX_ERR_STMT - 1] = '\0';
			}
			warn_or_exit_horribly(AH, modulename, "%s: %s    Command was: %s\n",
								  desc, PQerrorMessage(conn), errStmt);
			break;
	}

	PQclear(res);
}


/*
 * Process non-COPY table data (that is, INSERT commands).
 *
 * The commands have been run together as one long string for compressibility,
 * and we are receiving them in bufferloads with arbitrary boundaries, so we
 * have to locate command boundaries and save partial commands across calls.
 * All state must be kept in AH->sqlparse, not in local variables of this
 * routine.  We assume that AH->sqlparse was filled with zeroes when created.
 *
 * We have to lex the data to the extent of identifying literals and quoted
 * identifiers, so that we can recognize statement-terminating semicolons.
 * We assume that INSERT data will not contain SQL comments, E'' literals,
 * or dollar-quoted strings, so this is much simpler than a full SQL lexer.
 *
 * Note: when restoring from a pre-9.0 dump file, this code is also used to
 * process BLOB COMMENTS data, which has the same problem of containing
 * multiple SQL commands that might be split across bufferloads.  Fortunately,
 * that data won't contain anything complicated to lex either.
 */
static void
ExecuteSimpleCommands(ArchiveHandle *AH, const char *buf, size_t bufLen)
{
	const char *qry = buf;
	const char *eos = buf + bufLen;

	/* initialize command buffer if first time through */
	if (AH->sqlparse.curCmd == NULL)
		AH->sqlparse.curCmd = createPQExpBuffer();

	for (; qry < eos; qry++)
	{
		char		ch = *qry;

		/* For neatness, we skip any newlines between commands */
		if (!(ch == '\n' && AH->sqlparse.curCmd->len == 0))
			appendPQExpBufferChar(AH->sqlparse.curCmd, ch);

		switch (AH->sqlparse.state)
		{
			case SQL_SCAN:		/* Default state == 0, set in _allocAH */
				if (ch == ';')
				{
					/*
					 * We've found the end of a statement. Send it and reset
					 * the buffer.
					 */
					ExecuteSqlCommand(AH, AH->sqlparse.curCmd->data,
									  "could not execute query");
					resetPQExpBuffer(AH->sqlparse.curCmd);
				}
				else if (ch == '\'')
				{
					AH->sqlparse.state = SQL_IN_SINGLE_QUOTE;
					AH->sqlparse.backSlash = false;
				}
				else if (ch == '"')
				{
					AH->sqlparse.state = SQL_IN_DOUBLE_QUOTE;
				}
				break;

			case SQL_IN_SINGLE_QUOTE:
				/* We needn't handle '' specially */
				if (ch == '\'' && !AH->sqlparse.backSlash)
					AH->sqlparse.state = SQL_SCAN;
				else if (ch == '\\' && !AH->public.std_strings)
					AH->sqlparse.backSlash = !AH->sqlparse.backSlash;
				else
					AH->sqlparse.backSlash = false;
				break;

			case SQL_IN_DOUBLE_QUOTE:
				/* We needn't handle "" specially */
				if (ch == '"')
					AH->sqlparse.state = SQL_SCAN;
				break;
		}
	}
}

/*
 * Restore given table and its indexes, fsm and vm in transfer mode.
 * Also, generate wal files for each restored file, if necessary.
 */
void
doTransferRelRestore(ArchiveHandle *AH, TocEntry *te)
{
	Archive *AHX = (Archive *) AH;
	RestoreOptions *ropt = AHX->ropt;
	RelFileMap *map;
	RelFileMap *sequencemap;
	RelFileMap *toastmap;
	bool is_restore = true;
	bool copy_mode = ropt->copy_mode_transfer;
	bool is_verbose = ropt->verbose;
	int nrels;
	int nseqrels;
	int ntoastrels;
	int i;

	/* Find files to restore and map them to schema */
	map = fillRelFileMap(AHX, &nrels, &ntoastrels, ropt->dbname, fmtId(te->tag));
	sequencemap = fillRelFileMapSeq(AHX, &nseqrels, ropt->dbname, fmtId(te->tag));
	if (ntoastrels > 0)
		toastmap = fillRelFileMapToast(AHX, map, nrels, ntoastrels);

	/* Restore plain relations */
	for (i = 0; i < nrels; i++)
	{
		ahprintf(AH, "SELECT pg_transfer_cleanup_shmem(%u::oid);", (&map[i])->reloid);

		transfer_relfile(&map[i], "", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&map[i], "", ".cfm", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&map[i], "_fsm", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&map[i], "_vm", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		if (ropt->generate_wal)
			ahprintf(AH, "SELECT pg_transfer_wal(%u::oid);", (&map[i])->reloid);
	}

	/* Restore sequences */
	for (i = 0; i < nseqrels; i++)
	{
		ahprintf(AH, "SELECT pg_transfer_cleanup_shmem(%u::oid);", (&sequencemap[i])->reloid);

		transfer_relfile(&sequencemap[i], "", "",  ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&sequencemap[i], "", ".cfm",  ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		if (ropt->generate_wal)
			ahprintf(AH, "SELECT pg_transfer_wal(%u::oid);", (&sequencemap[i])->reloid);
	}

	/* Restore toast relations */
	for (i = 0; i < ntoastrels*2; i++)
	{
		ahprintf(AH, "SELECT pg_transfer_cleanup_shmem(%u::oid);", (&toastmap[i])->reloid);

		transfer_relfile(&toastmap[i], "", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&toastmap[i], "", ".cfm", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&toastmap[i], "_fsm", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		transfer_relfile(&toastmap[i], "_vm", "", ropt->transfer_dir,
						 is_restore, copy_mode, is_verbose);
		if (ropt->generate_wal)
			ahprintf(AH, "SELECT pg_transfer_wal(%u::oid);", (&toastmap[i])->reloid);
	}
}

/*
 * Implement ahwrite() for direct-to-DB restore
 */
int
ExecuteSqlCommandBuf(Archive *AHX, const char *buf, size_t bufLen)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;

	if (AH->outputKind == OUTPUT_COPYDATA)
	{
		/*
		 * COPY data.
		 *
		 * We drop the data on the floor if libpq has failed to enter COPY
		 * mode; this allows us to behave reasonably when trying to continue
		 * after an error in a COPY command.
		 */
		if (AH->pgCopyIn &&
			PQputCopyData(AH->connection, buf, bufLen) <= 0)
			exit_horribly(modulename, "error returned by PQputCopyData: %s",
						  PQerrorMessage(AH->connection));
	}
	else if (AH->outputKind == OUTPUT_OTHERDATA)
	{
		/*
		 * Table data expressed as INSERT commands; or, in old dump files,
		 * BLOB COMMENTS data (which is expressed as COMMENT ON commands).
		 */
		ExecuteSimpleCommands(AH, buf, bufLen);
	}
	else
	{
		/*
		 * General SQL commands; we assume that commands will not be split
		 * across calls.
		 *
		 * In most cases the data passed to us will be a null-terminated
		 * string, but if it's not, we have to add a trailing null.
		 */
		if (buf[bufLen] == '\0')
			ExecuteSqlCommand(AH, buf, "could not execute query");
		else
		{
			char	   *str = (char *) pg_malloc(bufLen + 1);

			memcpy(str, buf, bufLen);
			str[bufLen] = '\0';
			ExecuteSqlCommand(AH, str, "could not execute query");
			free(str);
		}
	}

	return bufLen;
}

/*
 * Terminate a COPY operation during direct-to-DB restore
 */
void
EndDBCopyMode(Archive *AHX, const char *tocEntryTag)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;

	if (AH->pgCopyIn)
	{
		PGresult   *res;

		if (PQputCopyEnd(AH->connection, NULL) <= 0)
			exit_horribly(modulename, "error returned by PQputCopyEnd: %s",
						  PQerrorMessage(AH->connection));

		/* Check command status and return to normal libpq state */
		res = PQgetResult(AH->connection);
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
			warn_or_exit_horribly(AH, modulename, "COPY failed for table \"%s\": %s",
								tocEntryTag, PQerrorMessage(AH->connection));
		PQclear(res);

		/* Do this to ensure we've pumped libpq back to idle state */
		if (PQgetResult(AH->connection) != NULL)
			write_msg(NULL, "WARNING: unexpected extra results during COPY of table \"%s\"\n",
					  tocEntryTag);

		AH->pgCopyIn = false;
	}
}

void
StartTransaction(Archive *AHX)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;

	ExecuteSqlCommand(AH, "BEGIN", "could not start database transaction");
}

void
CommitTransaction(Archive *AHX)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;

	ExecuteSqlCommand(AH, "COMMIT", "could not commit database transaction");
}

void
DropBlobIfExists(ArchiveHandle *AH, Oid oid)
{
	/*
	 * If we are not restoring to a direct database connection, we have to
	 * guess about how to detect whether the blob exists.  Assume new-style.
	 */
	if (AH->connection == NULL ||
		PQserverVersion(AH->connection) >= 90000)
	{
		ahprintf(AH,
				 "SELECT pg_catalog.lo_unlink(oid) "
				 "FROM pg_catalog.pg_largeobject_metadata "
				 "WHERE oid = '%u';\n",
				 oid);
	}
	else
	{
		/* Restoring to pre-9.0 server, so do it the old way */
		ahprintf(AH,
				 "SELECT CASE WHEN EXISTS("
				 "SELECT 1 FROM pg_catalog.pg_largeobject WHERE loid = '%u'"
				 ") THEN pg_catalog.lo_unlink('%u') END;\n",
				 oid, oid);
	}
}

/* fillRelFileMap. get all parts of filepath */
RelFileMap *
fillRelFileMap(Archive *fout, int *nrels, int *ntoastrels,
			   const char *dbname, const char *tblname)
{
	PQExpBuffer query;
	PGresult	*res;
	int			i_dboid;
	int 		ntups;
	int			i_oid;
	int			i_relfilenode;
	int			i_relname;
	int			i_tablespace;
	int			i_reltoastrelid;
	int			i_data_directory;
	int 		i_tablespace_location;
	int 		i_relpersistence;
	const char *data_directory;
	Oid			dboid;
	int i;
	RelFileMap *map;

	*nrels = *ntoastrels = 0;
	query = createPQExpBuffer();

	/*
	 * At first get $PGDATA.
	 */
	appendPQExpBuffer(query, "SHOW data_directory;");
	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);
	i_data_directory =  PQfnumber(res, "data_directory");
	data_directory = strdup(PQgetvalue(res, 0, i_data_directory));

	PQclear(res);
	resetPQExpBuffer(query);

	/*
	 * Next: get db_oid to construct filepath
	 */
	appendPQExpBuffer(query, "select oid from pg_database where datname = '%s'",
								dbname);
	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);

	i_dboid = PQfnumber(res, "oid");
	dboid= atooid(PQgetvalue(res, 0, i_dboid));

	PQclear(res);
	resetPQExpBuffer(query);

	/*
	 * Next: get relfilenode and reltablespace to construct filepath
	 */
	appendPQExpBuffer(query, "WITH tableoid AS (SELECT oid FROM pg_class WHERE relname = '%s'), \n"
					 "indoid AS (SELECT indexrelid FROM pg_index WHERE indrelid IN \n"
					 "(SELECT oid FROM tableoid)) \n"
					 "SELECT oid, pg_tablespace_location(reltablespace), relname, relfilenode, \n"
					 "reltablespace, reltoastrelid, relpersistence FROM pg_class WHERE \n"
					 "oid IN (SELECT oid FROM tableoid) OR \n"
					 "oid IN (SELECT indexrelid FROM indoid)", tblname);

	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);
	ntups = PQntuples(res);

	if (ntups == 0)
		exit_horribly(NULL, "Relation '%s' is not found \n", tblname);

	*nrels = ntups;

	i_oid = PQfnumber(res, "oid");
	i_relfilenode = PQfnumber(res, "relfilenode");
	i_relname = PQfnumber(res, "relname");
	i_tablespace = PQfnumber(res, "reltablespace");
	i_reltoastrelid = PQfnumber(res, "reltoastrelid");
	i_tablespace_location = PQfnumber(res, "pg_tablespace_location");
	i_relpersistence = PQfnumber(res, "relpersistence");

	map = pg_malloc(sizeof(RelFileMap) * ntups);

	for (i = 0; i < ntups; i++)
	{
		RelFileMap *cur = map + i;

		cur->db_oid = dboid;

		cur->reloid = atooid(PQgetvalue(res, i, i_oid));
		cur->relfilenode = atooid(PQgetvalue(res, i, i_relfilenode));
		cur->relname = strdup(PQgetvalue(res, i, i_relname));
		cur->reltoastrelid = atooid(PQgetvalue(res, i, i_reltoastrelid));

		/* 'c' is for RELPERSISTENCE_CONSTANT */
		if (*PQgetvalue(res, i, i_relpersistence) != 'c')
			exit_horribly(NULL, "Cannot transfer non-constant relation '%s' \n",
						  cur->relname);

		if (cur->reltoastrelid != 0)
			*ntoastrels += 1;

		if (atoi(PQgetvalue(res, i, i_tablespace)) == 0)
		{
			cur->datadir = strdup(data_directory);
			cur->tablespace = strdup("/base");
		}
		else
		{
			char tblsp_path[MAXPGPATH];

			#define CATALOG_VERSION_NO	201608191
			#define TABLESPACE_VERSION_DIRECTORY	"PG_" PG_MAJORVERSION "_" \
										CppAsString2(CATALOG_VERSION_NO)
			/*
			 * We need special treatment for tablespaces other than pg_global.
			 * There are symllinks stored in the tablespce dir,
			 * but we need to get locations of real files.
			 */
			cur->datadir = strdup("");
			snprintf(tblsp_path, sizeof(tblsp_path),
					 "%s/%s", PQgetvalue(res, i, i_tablespace_location), TABLESPACE_VERSION_DIRECTORY);
			cur->tablespace = strdup(tblsp_path);
		}
	}

	PQclear(res);
	destroyPQExpBuffer(query);
	return map;
}

RelFileMap *
fillRelFileMapSeq(Archive *fout, int *nrels,
			   const char *dbname, const char *tblname)
{
	PQExpBuffer query;
	PGresult	*res;
	int			i_dboid;
	int 		ntups;
	int			i_oid;
	int			i_relfilenode;
	int			i_relname;
	int			i_tablespace;
	int			i_data_directory;
	int 		i_tablespace_location;
	const char *data_directory;
	Oid			dboid;
	int i;
	RelFileMap *map;

	*nrels = 0;
	query = createPQExpBuffer();

	/*
	 * At first get $PGDATA.
	 */
	appendPQExpBuffer(query, "SHOW data_directory;");
	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);
	i_data_directory =  PQfnumber(res, "data_directory");
	data_directory = strdup(PQgetvalue(res, 0, i_data_directory));

	PQclear(res);
	resetPQExpBuffer(query);

	/*
	 * Next: get db_oid to construct filepath
	 */
	appendPQExpBuffer(query, "select oid from pg_database where datname = '%s'",
								dbname);
	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);

	i_dboid = PQfnumber(res, "oid");
	dboid= atooid(PQgetvalue(res, 0, i_dboid));

	PQclear(res);
	resetPQExpBuffer(query);

	/*
	 * Find all sequences related to the table,
	 * add them to the list
	 */
	appendPQExpBuffer(query, "SELECT s.oid, pg_tablespace_location(s.reltablespace), s.relname, s.relfilenode, \n"
								"s.reltablespace, s.relpersistence \n"
					 "FROM pg_class s  \n"
					 "JOIN pg_depend d ON d.objid=s.oid AND d.classid='pg_class'::regclass  \n"
					 "AND d.refclassid='pg_class'::regclass \n"
					 "JOIN pg_class t ON t.oid=d.refobjid \n"
					 "JOIN pg_namespace n on n.oid=t.relnamespace \n"
					 "JOIN pg_attribute a on a.attrelid=t.oid AND a.attnum=d.refobjsubid \n"
					 "WHERE s.relkind='S' AND d.deptype='a' AND t.relname = '%s' \n", tblname);
	res = ExecuteSqlQuery(fout, query->data, PGRES_TUPLES_OK);
	ntups = PQntuples(res);

	if (ntups == 0)
		return NULL;
	*nrels = ntups;

	i_oid = PQfnumber(res, "oid");
	i_relfilenode = PQfnumber(res, "relfilenode");
	i_relname = PQfnumber(res, "relname");
	i_tablespace = PQfnumber(res, "reltablespace");
	i_tablespace_location = PQfnumber(res, "pg_tablespace_location");
	map = pg_malloc(sizeof(RelFileMap) * ntups);

	for (i = 0; i < ntups; i++)
	{
		RelFileMap *cur = map + i;

		cur->db_oid = dboid;
		cur->reloid = atooid(PQgetvalue(res, i, i_oid));
		cur->relfilenode = atooid(PQgetvalue(res, i, i_relfilenode));
		cur->relname = strdup(PQgetvalue(res, i, i_relname));

		if (atoi(PQgetvalue(res, i, i_tablespace)) == 0)
		{
			cur->datadir = strdup(data_directory);
			cur->tablespace = strdup("/base");
		}
		else
		{
			char tblsp_path[MAXPGPATH];

			#define CATALOG_VERSION_NO	201608191
			#define TABLESPACE_VERSION_DIRECTORY	"PG_" PG_MAJORVERSION "_" \
										CppAsString2(CATALOG_VERSION_NO)
			/*
			 * We need special treatment for tablespaces other than pg_global.
			 * There are symllinks stored in the tablespce dir,
			 * but we need to get locations of real files.
			 */
			cur->datadir = strdup("");
			snprintf(tblsp_path, sizeof(tblsp_path),
					 "%s/%s", PQgetvalue(res, i, i_tablespace_location), TABLESPACE_VERSION_DIRECTORY);
			cur->tablespace = strdup(tblsp_path);
		}
	}

	PQclear(res);
	destroyPQExpBuffer(query);
	return map;
}

/*
 * fillRelFileMapToast()
 * 		find all toast relations and fill mapping for them
 */
RelFileMap *
fillRelFileMapToast(Archive *fout, RelFileMap *map,
					int nrels, int ntoastrels)
{
	PGresult   *res;
	PQExpBuffer query;
	int			i_oid;
	int			i_relfilenode;
	int i;
	RelFileMap *toastmap;
	RelFileMap *curtoast;

	/* Each toast table has an index, so double toastmap size */
	toastmap = pg_malloc(sizeof(RelFileMap) * ntoastrels*2);

	query = createPQExpBuffer();

	/* fill mapping for toast rel */
	curtoast = toastmap;

	for (i = 0; i < nrels; i++)
	{
		if(map[i].reltoastrelid != 0)
		{
			/* Next: get relfilenode of toast table to construct filepath */
			appendPQExpBuffer(query, "SELECT relname, relfilenode FROM pg_class WHERE \n"
									"oid = %d",
									map[i].reltoastrelid);
			res = ExecuteSqlQueryForSingleRow(fout, query->data);

			if (PQntuples(res) == 0)
				exit_horribly(NULL, "No toast relation with oid %d \n",map[i].reltoastrelid);

			i_relfilenode = PQfnumber(res, "relfilenode");

			curtoast->datadir = strdup(map[i].datadir);
			curtoast->tablespace = strdup(map[i].tablespace);
			curtoast->db_oid = map[i].db_oid;

			curtoast->reloid = map[i].reltoastrelid;
			curtoast->relfilenode = atooid(PQgetvalue(res, 0, i_relfilenode));

			curtoast->relname = palloc(NAMEDATALEN);
			sprintf(curtoast->relname, "pg_toast_%s", map[i].relname);

			PQclear(res);
			resetPQExpBuffer(query);

			/* Next: get relfilenode of toast index to construct filepath */
			curtoast += 1;

			appendPQExpBuffer(query, "WITH indoid AS (SELECT indexrelid FROM pg_index WHERE indrelid = %d) \n"
									  "SELECT oid, relname, relfilenode  FROM pg_class \n"
									  "WHERE oid IN (SELECT indexrelid FROM indoid)",
									  map[i].reltoastrelid);
			res = ExecuteSqlQueryForSingleRow(fout, query->data);

			if (PQntuples(res) == 0)
				exit_horribly(NULL, "No toast index relation with oid %d \n",map[i].reltoastrelid);

			curtoast->datadir = strdup(map[i].datadir);
			curtoast->tablespace = strdup(map[i].tablespace);
			curtoast->db_oid = map[i].db_oid;

			i_oid = PQfnumber(res, "oid");
			i_relfilenode = PQfnumber(res, "relfilenode");

			curtoast->reloid = atooid(PQgetvalue(res, 0, i_oid));
			curtoast->relfilenode = atooid(PQgetvalue(res, 0, i_relfilenode));

			curtoast->relname = palloc(NAMEDATALEN);
			sprintf(curtoast->relname, "pg_toast_%s_index", map[i].relname);

			PQclear(res);
			resetPQExpBuffer(query);

			curtoast += 1;
		}
	}

	destroyPQExpBuffer(query);

	return toastmap;
}

static int
copy_file(const char *srcfile, const char *dstfile, bool create_file)
{
#ifndef WIN32
#define COPY_BUF_SIZE (50 * BLCKSZ)

	int			src_fd;
	int			dest_fd;
	char	   *buffer;
	int			ret = 0;
	int			save_errno = 0;
	int dest_flags = 0;

	if ((srcfile == NULL) || (dstfile == NULL))
	{
		errno = EINVAL;
		return -1;
	}

	if ((src_fd = open(srcfile, O_RDONLY, 0)) < 0)
		return -1;

	if (create_file)
		dest_flags = O_RDWR | O_CREAT | O_EXCL;
	else
		dest_flags = O_RDWR | O_CREAT;

	if ((dest_fd = open(dstfile, dest_flags, S_IRUSR | S_IWUSR)) < 0)
	{
		save_errno = errno;

		if (src_fd != 0)
			close(src_fd);

		errno = save_errno;
		return -1;
	}

	buffer = (char *) pg_malloc(COPY_BUF_SIZE);

	/* perform data copying i.e read src source, write to destination */
	while (true)
	{
		ssize_t		nbytes = read(src_fd, buffer, COPY_BUF_SIZE);

		if (nbytes < 0)
		{
			save_errno = errno;
			ret = -1;
			break;
		}

		if (nbytes == 0)
			break;

		errno = 0;

		if (write(dest_fd, buffer, nbytes) != nbytes)
		{
			/* if write didn't set errno, assume problem is no disk space */
			if (errno == 0)
				errno = ENOSPC;
			save_errno = errno;
			ret = -1;
			break;
		}
	}

	pg_free(buffer);

	if (src_fd != 0)
		close(src_fd);

	if (dest_fd != 0)
		close(dest_fd);

	if (save_errno != 0)
		errno = save_errno;

	return ret;
#else	/* WIN32 */

	/* Last argument of Windows CopyFile func is bFailIfExists.
	 * If we're asked to create_file, it should not exist. Otherwise, we
	 * want to rewrite it.
	 */
	if (CopyFile(srcfile, dstfile, create_file) == 0)
	{
		_dosmaperr(GetLastError());
		return -1;
	}
	return 0;

#endif	/* WIN32 */
}

#ifdef WIN32
/* implementation of pg_link_file() on Windows */
static int
win32_pghardlink(const char *src, const char *dst)
{
	/*
	 * CreateHardLinkA returns zero for failure
	 * http://msdn.microsoft.com/en-us/library/aa363860(VS.85).aspx
	 */
	if (CreateHardLinkA(dst, src, NULL) == 0)
	{
		_dosmaperr(GetLastError());
		return -1;
	}
	else
		return 0;
}
#endif

/*
 * Transfer relation fork, specified by type_suffix, to transfer subdir.
 * In copy mode, just move relfiles to transfer_dir (if dump) or
 * from transfer_dir to database (if restore).
 */
void
transfer_relfile(RelFileMap *map, const char *type_suffix,
				 const char *cfm_suffix,
				 const char *transfer_dir, bool is_restore,
				 bool is_copy_mode, bool is_verbose)
{
	char		db_file[MAXPGPATH];
	char		transfer_file[MAXPGPATH];
	int			segno;
	char		extent_suffix[65];
	struct stat statbuf;

	/*
	 * Now copy/link any related segments as well. Remember, PG breaks large
	 * files into 1GB segments, the first segment has no extension, subsequent
	 * segments are named relfilenode.1, relfilenode.2, relfilenode.3. copied.
	 */
	for (segno = 0;; segno++)
	{
		if (segno == 0)
			extent_suffix[0] = '\0';
		else
			snprintf(extent_suffix, sizeof(extent_suffix), ".%d", segno);

		snprintf(db_file, sizeof(db_file), "%s%s/%u/%u%s%s%s",
				 map->datadir,
				 map->tablespace,
				 map->db_oid,
				 map->relfilenode,
				 type_suffix,
				 extent_suffix,
				 cfm_suffix);
		snprintf(transfer_file, sizeof(transfer_file), "%s%s%s%s%s",
				 transfer_dir,
				 map->relname,
				 type_suffix,
				 extent_suffix,
				 cfm_suffix);

		/* Did file open fail? */
		if (!is_restore)
		{
			if (stat(db_file, &statbuf) != 0)
			{
				/*
				 * vm, fsm, cfm or non-first segment file does not exist?
				 * That's OK, just return
				 */
				if (type_suffix[0] != '\0'
					|| cfm_suffix[0] != '\0'
					|| segno != 0)
				{
					if (errno == ENOENT)
						return;
				}
				exit_horribly(NULL, "error while checking for file existence \"%s\" (\"%s\" to \"%s\")\n",
									map->relname, db_file, transfer_file);
			}
		}
		else
		{
			if (stat(transfer_file, &statbuf) != 0)
			{
				/*
				 * vm, fsm, cfm or non-first segment file does not exist?
				 * That's OK, just return
				 */
				if (type_suffix[0] != '\0'
					|| cfm_suffix[0] != '\0'
					|| segno != 0)
				{
					if (errno == ENOENT)
						return;
				}
				exit_horribly(NULL, "error while checking for file existence \"%s\" (\"%s\" to \"%s\")\n",
									map->relname, transfer_file, db_file);
			}

			if ((statbuf.st_nlink > 1) && (!is_copy_mode))
				exit_horribly(NULL, "file \"%s\" has more than one link.\n"
							"You're probably trying to restore files on the same filesystem "
							"they were dumped.\nIt is not allowed.\n Use --copy-mode-transfer option.\n", transfer_file);
		}

		/* We do symmetric actions on dump and restore. */
		if (!is_restore)
		{
			/* If dump, transfer file from database to transfer_dir */
			if (is_verbose)
				write_msg(NULL, "dump file (%s) \"%s\" to \"%s\"\n",
					is_copy_mode?"copy":"link", db_file, transfer_file);

			if (is_copy_mode)
			{
				if (copy_file(db_file, transfer_file, true) != 0)
				{
					exit_horribly(NULL, "cannot dump file (copy mode) %s to %s : %s \n",
								db_file, transfer_file, strerror(errno));
				}
			}
			else
			{
				if (pg_link_file(db_file, transfer_file) != 0)
				{
					exit_horribly(NULL, "cannot dump file (link mode) %s to %s : %s \n",
								db_file, transfer_file, strerror(errno));
				}
			}
		}
		else
		{
			/* If restore, transfer file from transfer_dir to database */
			if (is_verbose)
				write_msg(NULL, "restore file (%s) \"%s\" to \"%s\"\n",
					is_copy_mode?"copy":"link", transfer_file, db_file);

			if (is_copy_mode)
			{
				if (copy_file(transfer_file, db_file, false) != 0)
				{
					exit_horribly(NULL, "cannot restore file (copy mode) %s to %s : %s \n",
								transfer_file, db_file, strerror(errno));
				}
			}
			else
			{
				if (rename(transfer_file, db_file) != 0)
				{
					exit_horribly(NULL, "cannot restore file (rename mode) %s to %s : %s \n",
								transfer_file, db_file, strerror(errno));
				}
			}
		}
	}
	return;
}

/*
 * dumpControlData: put the info from pg_control into
 * the special file in transfer directory
 */
void
transferCheckControlData(Archive *fout, const char *transfer_dir, bool isRestore)
{
	int		fd;
	char		controlefilepath[MAXPGPATH];
	PQExpBuffer q = createPQExpBuffer();
	PGresult   *res;
	const char *serverInfo;

	appendPQExpBuffer(q, "SELECT pg_control_init();");
	res = ExecuteSqlQueryForSingleRow(fout, q->data);
	serverInfo = pg_strdup(PQgetvalue(res, 0, 0));
	destroyPQExpBuffer(q);

	snprintf(controlefilepath, sizeof(controlefilepath), "%spg_control.init", transfer_dir);

	if (!isRestore)
	{
		fd = open(controlefilepath,
				O_RDWR | O_CREAT | O_EXCL | PG_BINARY,
					   S_IRUSR | S_IWUSR);
		if (fd < 0)
			exit_horribly(NULL,"could not create control file dump \"%s\" \n",
						controlefilepath);

		errno = 0;
		/* In dump mode copy info to the pg_control in transfer_dir */
		if (write(fd, serverInfo, strlen(serverInfo)) != strlen(serverInfo))
		{
			/* if write didn't set errno, assume problem is no disk space */
			if (errno == 0)
				errno = ENOSPC;
			exit_horribly(NULL,"could not write to control file dump");
		}

		if (fsync(fd) != 0)
			exit_horribly(NULL,"could not fsync control file dump");
	}
	else
	{
#ifdef __GNUC__		
		char dumpedInfo[strlen(serverInfo)];
#else	
		char dumpedInfo[1024];
#endif		
		/*
		 * In restore mode read info from pg_control in transfer_dir
		 * and compare it with the result of select. In case of any
		 * inconsistensy throw an error and stop restore process.
		 */

		fd = open(controlefilepath, O_RDONLY | PG_BINARY, 0);
		if (fd < 0)
			exit_horribly(NULL,"could not open control file dump \"%s\" \n",
						controlefilepath);

		if (read(fd, dumpedInfo, strlen(serverInfo)) != strlen(serverInfo))
			exit_horribly(NULL,"could not read control file dump \n");

		if (strncmp(serverInfo, dumpedInfo, strlen(serverInfo)) != 0)
			exit_horribly(NULL,"could not read control file dump. dumpedInfo: %s \n serverInfo: %s\n",
						  dumpedInfo, serverInfo);
	}

	if (close(fd))
		exit_horribly(NULL,"could not close control file dump \n");
}

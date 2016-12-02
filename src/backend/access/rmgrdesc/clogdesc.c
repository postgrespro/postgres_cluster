/*-------------------------------------------------------------------------
 *
 * clogdesc.c
 *	  rmgr descriptor routines for access/transam/clog.c
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/access/rmgrdesc/clogdesc.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/clog.h"


void
clog_desc(StringInfo buf, XLogReaderState *record)
{
	char	   *rec = XLogRecGetData(record);
	uint8		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

	if (info == CLOG_ZEROPAGE || info == CLOG_TRUNCATE)
	{
		int64		pageno;

		memcpy(&pageno, rec, sizeof(int64));
		appendStringInfo(buf, INT64_FORMAT, pageno);
	}
}

const char *
clog_identify(uint8 info)
{
	const char *id = NULL;

	switch (info & ~XLR_INFO_MASK)
	{
		case CLOG_ZEROPAGE:
			id = "ZEROPAGE";
			break;
		case CLOG_TRUNCATE:
			id = "TRUNCATE";
			break;
	}

	return id;
}

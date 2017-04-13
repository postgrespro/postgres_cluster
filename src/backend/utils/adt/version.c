/*-------------------------------------------------------------------------
 *
 * version.c
 *	 Returns the PostgreSQL version string
 *
 * Copyright (c) 1998-2016, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *
 * src/backend/utils/adt/version.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/builtins.h"

#include "commit_id.h"

Datum
pgsql_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(PG_VERSION_STR));
}

Datum
pgpro_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(PGPRO_VERSION_STR));
}

Datum
pgpro_edition(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(PGPRO_EDITION));
}	

Datum
pgpro_build(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(COMMIT_ID));
}

/*-------------------------------------------------------------------------
 *
 * pglogical_relid_map.c
 *		  Logical Replication map of local Oids to to remote
 *
 * Copyright (c) 2012-2015, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  pglogical_relid_map.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/hsearch.h"
#include "pglogical_relid_map.h"

static HTAB *relid_map;

static void
pglogical_relid_map_init(void)
{
	HASHCTL	ctl;
	Assert(relid_map == NULL);

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(PGLRelidMapEntry);
	relid_map = hash_create("pglogical_relid_map", PGL_INIT_RELID_MAP_SIZE, &ctl, HASH_ELEM | HASH_BLOBS);

	Assert(relid_map != NULL);
}

Oid pglogical_relid_map_get(Oid relid)
{
	if (relid_map != NULL) {
		PGLRelidMapEntry* entry = (PGLRelidMapEntry*)hash_search(relid_map, &relid, HASH_FIND, NULL);
		return entry ? entry->local_relid : InvalidOid;
	}
	return InvalidOid;
}

bool pglogical_relid_map_put(Oid remote_relid, Oid local_relid)
{
	bool found;
    PGLRelidMapEntry* entry;
    if (relid_map == NULL) {
        pglogical_relid_map_init();
    }
    entry = hash_search(relid_map, &remote_relid, HASH_ENTER, &found);
  	if (found) {
	    entry->local_relid = local_relid;
		return false;
    }
    entry->local_relid = local_relid;
	return true;
}

void pglogical_relid_map_reset(void)
{
	if (relid_map != NULL) {
		hash_destroy(relid_map);
		relid_map = NULL;
	}
}

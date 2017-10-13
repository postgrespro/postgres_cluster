#ifndef PTRACK_H
#define PTRACK_H

#include "access/xlogdefs.h"
#include "storage/block.h"
#include "storage/buf.h"
#include "storage/relfilenode.h"
#include "utils/relcache.h"

/* Ptrack version as a string */
#define PTRACK_VERSION "1.3"
/* Ptrack version as a number */
#define PTRACK_VERSION_NUM 103

/* Number of bits allocated for each heap block. */
#define PTRACK_BITS_PER_HEAPBLOCK 1

#define PTRACK_INIT_FILE "ptrack_init"

extern PGDLLIMPORT bool ptrack_enable;

extern void ptrack_add_block(Relation rel, BlockNumber heapBlk);
extern void ptrack_add_block_redo(RelFileNode rnode, BlockNumber heapBlk);
extern void ptrack_pin(Relation rel, BlockNumber heapBlk, Buffer *buf);
extern void ptrack_set(BlockNumber heapBlk, Buffer ptrackBuf);
extern void create_ptrack_init_file(char *dest_dir);
extern void drop_ptrack_init_file(char *dest_dir);

extern void ptrack_clear(void);
extern bytea *ptrack_get_and_clear(Oid tablespace_oid, Oid table_oid);
extern void assign_ptrack_enable(bool newval, void *extra);

#endif   /* PTRACK_H */

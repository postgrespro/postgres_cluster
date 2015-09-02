#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"

#define INVALID_GCID 0
#define COMMIT_UNKNOWN 0
#define COMMIT_YES     1
#define COMMIT_NO      2

typedef unsigned long long cid_t;

typedef struct DTMConnData *DTMConn;

// Connects to the specified DTM.
DTMConn DtmConnect(char *host, int port);

// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
void DtmDisconnect(DTMConn dtm);

// Asks DTM for the first 'gcid' with unknown status. This 'gcid' is used as a
// kind of snapshot. Returns the 'gcid' on success, or INVALID_GCID otherwise.
cid_t DtmGlobalGetNextCid(DTMConn dtm);

// Prepares a commit. Returns the 'gcid' on success, or INVALID_GCID otherwise.
cid_t DtmGlobalPrepare(DTMConn dtm);

// Finishes a given commit with 'committed' status. Returns 'true' on success,
// 'false' otherwise.
bool DtmGlobalCommit(DTMConn dtm, cid_t gcid);

// Finishes a given commit with 'aborted' status.
void DtmGlobalRollback(DTMConn dtm, cid_t gcid);

// Gets the status of the commit identified by 'gcid'. Returns the status on
// success, or -1 otherwise.
int DtmGlobalGetTransStatus(DTMConn dtm, cid_t gcid);

#endif

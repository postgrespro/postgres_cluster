#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"
#include "utils/snapshot.h"

#define INVALID_GXID 0
#define COMMIT_UNKNOWN 0
#define COMMIT_YES     1
#define COMMIT_NO      2

typedef unsigned long long xid_t;

typedef struct DTMConnData *DTMConn;

// Connects to the specified DTM.
DTMConn DtmConnect(char *host, int port);

// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
void DtmDisconnect(DTMConn dtm);

// Asks DTM for a fresh snapshot. Returns a snapshot on success, or NULL
// otherwise. Please free the snapshot memory yourself after use.
Snapshot DtmGlobalGetSnapshot(DTMConn dtm);

// Starts a transaction. Returns the 'gxid' on success, or INVALID_GXID otherwise.
xid_t DtmGlobalBegin(DTMConn dtm);

// Marks a given transaction as 'committed'. Returns 'true' on success,
// 'false' otherwise.
bool DtmGlobalCommit(DTMConn dtm, xid_t gxid);

// Marks a given transaction as 'aborted'.
void DtmGlobalRollback(DTMConn dtm, xid_t gxid);

// Gets the status of the commit identified by 'gxid'. Returns the status on
// success, or -1 otherwise.
int DtmGlobalGetTransStatus(DTMConn dtm, xid_t gxid);

#endif

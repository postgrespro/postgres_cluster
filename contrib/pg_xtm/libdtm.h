#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"
#include "utils/snapmgr.h"
#include "access/clog.h"

#define INVALID_XID 0
#define COMMIT_UNKNOWN 0
#define COMMIT_YES     1
#define COMMIT_NO      2

typedef int NodeId;
typedef unsigned long long xid_t;

typedef struct DTMConnData *DTMConn;

// Connects to the specified DTM.
DTMConn DtmConnect(char *host, int port);

// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
void DtmDisconnect(DTMConn dtm);


typedef struct {
    TransactionId* xids;
    NodeId* nodes;
    int nNodes;
} GlobalTransactionId;

/* create entry for new global transaction */
void DtmGlobalStartTransaction(DTMConn dtm, GlobalTransactionId* gtid);

Snapshot DtmGlobalGetSnapshot(DTMConn dtm, NodeId nodeid, TransactionId xid, Snapshot snapshot);

void DtmGlobalSetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, XidStatus status); /* commit transaction only once all participants are committed, before it do not change CLOG  */

XidStatus DtmGlobalGetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid);

#endif

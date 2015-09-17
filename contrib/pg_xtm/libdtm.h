#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"
#include "utils/snapmgr.h"
#include "storage/procarray.h"
#include "access/clog.h"

#define INVALID_XID 0

typedef int NodeId;
typedef unsigned long long xid_t;

typedef struct DTMConnData *DTMConn;

// Connects to the specified DTM.
DTMConn DtmConnect(char *host, int port);

// Disconnects from the DTM. Do not use the 'dtm' pointer after this call, or
// bad things will happen.
void DtmDisconnect(DTMConn dtm);

void DtmInitSnapshot(Snapshot snapshot);

typedef struct {
    TransactionId* xids;
    NodeId* nodes;
    int nNodes;
} GlobalTransactionId;

// Creates an entry for a new global transaction. Returns 'true' on success, or
// 'false' otherwise.
bool DtmGlobalStartTransaction(DTMConn dtm, GlobalTransactionId* gtid);

// Asks DTM for a fresh snapshot. Returns 'true' on success, or 'false'
// otherwise.
bool DtmGlobalGetSnapshot(DTMConn dtm, NodeId nodeid, TransactionId xid, Snapshot snapshot);

// Commits transaction only once all participants have called this function,
// does not change CLOG otherwise. Returns 'true' on success, 'false' if
// something failed on the daemon side.
bool DtmGlobalSetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, XidStatus status, bool wait);

// Gets the status of the transaction identified by 'xid'. Returns the status
// on success, or -1 otherwise. If 'wait' is true, then it does not return
// until the transaction is finished.
XidStatus DtmGlobalGetTransStatus(DTMConn dtm, NodeId nodeid, TransactionId xid, bool wait);

#endif

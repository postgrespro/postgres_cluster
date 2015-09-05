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


typedef struct {
    TransasactionId* xids;
    int nXids;
} GlobalTransactionId;

/* create entry for new global transaction */
void DtmGlobalStartTransaction(DTMConn dtm, TransactionId* gitd); 

void DtmGlobalGetSnapshot(DTMConn dtm, TransactionId xid, Snapshot snapshot);

void DtmGlobalSetTransStatus(DTMConn dtm, TransactionId xid, XidStatus status); /* commit transaction only once all participants are committed, before it do not change CLOG  */

XidStatus DtmGlobalGetTransStatus(DTMConn dtm, TransactionId xid);

#endif

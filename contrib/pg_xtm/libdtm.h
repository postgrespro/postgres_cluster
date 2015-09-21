#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"
#include "utils/snapmgr.h"
#include "storage/procarray.h"
#include "access/clog.h"

#define INVALID_XID 0

void DtmInitSnapshot(Snapshot snapshot);

// Starts new global transaction
TransactionId DtmGlobalStartTransaction(int nParticipants, Snapshot shaposhot);

// Asks DTM for a fresh snapshot.
void DtmGlobalGetSnapshot(TransactionId xid, Snapshot snapshot);

// Commits transaction only once all participants have called this function,
// does not change CLOG otherwise.
void DtmGlobalSetTransStatus(TransactionId xid, XidStatus status, bool wait);

// Gets the status of the transaction identified by 'xid'. Returns the status
// on success, or -1 otherwise. If 'wait' is true, then it does not return
// until the transaction is finished.
XidStatus DtmGlobalGetTransStatus(TransactionId xid, bool wait);

// Reserve XIDs for local transaction
TransactioinId DtmGlobalReserve(int nXids);   


#endif

#ifndef LIBDTM_H
#define LIBDTM_H

#include "postgres.h"
#include "utils/snapmgr.h"
#include "storage/procarray.h"
#include "access/clog.h"

#define INVALID_XID 0

void DtmInitSnapshot(Snapshot snapshot);

// Starts a new global transaction of nParticipants size. Returns the
// transaction id and fills the snapshot on success. Returns INVALID_XID
// otherwise.
TransactionId DtmGlobalStartTransaction(int nParticipants, Snapshot shapshot);

// Asks the DTM for a fresh snapshot.
void DtmGlobalGetSnapshot(TransactionId xid, Snapshot snapshot);

// Commits transaction only once all participants have called this function,
// does not change CLOG otherwise. Set 'wait' to 'true' if you want this call
// to return only after the transaction is considered finished by the DTM.
// Returns the status on success, or -1 otherwise.
XidStatus DtmGlobalSetTransStatus(TransactionId xid, XidStatus status, bool wait);

// Gets the status of the transaction identified by 'xid'. Returns the status
// on success, or -1 otherwise. If 'wait' is true, then it does not return
// until the transaction is finished.
XidStatus DtmGlobalGetTransStatus(TransactionId xid, bool wait);

// Reserves at least 'nXids' successive xids for local transactions. The xids
// reserved are not less than 'xid' in value. Returns the actual number
// of xids reserved, and sets the 'first' xid accordingly. The number of xids
// reserved is guaranteed to be at least nXids.
// In other words, *first ≥ xid and result ≥ nXids.
int DtmGlobalReserve(TransactionId xid, int nXids, TransactionId *first);

#endif

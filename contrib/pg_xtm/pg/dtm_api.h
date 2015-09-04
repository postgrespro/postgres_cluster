#ifndef DTM_API_H
#define DTM_API_H

typedef uint64 cid_t;

/* Check status of global transaction using CLOG at DTM */
TransactionIdStatus DtmGlobalGetTransStatus(cid_t gcid);
/* Get next CSN which will be assigned by DTM (used to build global snapshot) */
cid_t DtmGlobalGetNextCid();
/* Assign new CSN to global transaction at start of two phase commit */
cid_t DtmGlobalPrepare();
/* Mark global transaction as committed */
void DtmGlobalCommit(cid_t gcid);
/* Mark global transaction as aborted */
void DtmGlobalRollback(cid_t gcid);

#endif

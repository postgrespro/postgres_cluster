#ifndef DTM_BACKEND_H
#define DTM_BACKEND_H

#define MAX_GTID_SIZE  64

typedef int nodeid_t;
typedef uint64 cid_t;
typedef cid_t GlobalCSN;

typedef struct
{
	cid_t		snapshot;
	GlobalCSN	csn;
	char		gtid[MAX_GTID_SIZE];
	TransactionId xid;
	bool		is_global;
}	DtmCurrentTrans;

typedef char const *GlobalTransactionId;

Size		GlobalSnapshotShmemSize(void);

extern DtmCurrentTrans dtm_tx;

#define InvalidGlobalCSN	 0x0
#define AbortedGlobalCSN	 0x2
#define InDoubtGlobalCSN	 0x3
#define FirstNormalGlobalCSN 0x4

#define GlobalCSNIsNormal(csn) ((csn) >= FirstNormalGlobalCSN)

/* Initialize DTM extension */
void		DtmInitialize(void);

/* Invoked at start of any local or global transaction */
void		DtmLocalBegin(DtmCurrentTrans * x);

/* Extend local transaction to global by assigning upper bound CSN which is returned to coordinator */
extern cid_t		DtmLocalExtend(GlobalTransactionId gtid);

/* Function called at first access to any datanode except first one involved in distributed transaction */
cid_t		DtmLocalAccess(DtmCurrentTrans * x, GlobalTransactionId gtid, cid_t snapshot);

/* Mark transaction as in-doubt */
void		DtmLocalBeginPrepare(GlobalTransactionId gtid);

/* Choose CSN for global transaction */
cid_t		DtmLocalPrepare(GlobalTransactionId gtid, cid_t cid);

/* Assign CSN to global transaction */
void		DtmLocalEndPrepare(GlobalTransactionId gtid, cid_t cid);

/* Save global preapred transactoin state */
void		DtmLocalSavePreparedState(DtmCurrentTrans * x);

cid_t		DtmGetCsn(TransactionId xid);

#endif

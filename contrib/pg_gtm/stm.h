typedef struct OpaqueSnapshot* Snapshot;

typedef struct 
{
        void (*StartTransaction)(); // xact.c:StartTransacton
        void (*CommitTransaction)(); // xact.c:CommitTransacton
        void (*AbortTransaction)(); // xact.c:AbortTransacton
        void (*CleanupTransaction)(); // xact.c:CleanupTransacton

        void (*StartSubTransaction)(); // xact.c:StartSubTransacton
        void (*CommitSubTransaction)(); // xact.c:CommitSubTransacton
        void (*AbortSubTransaction)(); // xact.c:CommitTSubransacton
        void (*CleanupSubTransaction)(); // xact.c:CleanupSubTransacton

        void (*PushTransaction)(); // xact.c:PushTransaction
        void (*PopTransaction)(); // xact.c:PopTransaction

        void (*PrepareTransaction)(char *gid ); // xact.c:PrepareTransacton

        void (*AssignTransactionId)();// xact.c:AssignTransactionId
        TransactionId (*GetCurrentTransactionId)(); // xact.c:GetCurrentTransactionId
        TransactionId (*GetNextTransactionId)(); // varsup.c:ReadNewTransactionId
        
        bool (*IsCommitted)(TransactionId xid); // transam.c:TransactionIdDidCommit
        bool (*IsAborted)(TransactionId xid);   // transam.c:TransactionIdDidAbort
        bool (*IsCompleted)(TransactionId xid); // transam.c:TransactionIdIsKnownCompleted

        // Do we need them? 
        // Seems to be encapsulated by Commit/Rollback, but may be convenient to redefine just these functions
        void (*MarkCommitted)(TransactionId xid, int nxids, TransactionId *xids); // transam.c:TransactionIdCommitTree 
        void (*AsyncMarkCommitted)(TransactionId xid, int nxids, TransactionId *xids, XLogRecPtr lsn)); // transam.c:TransactionIdAsyncCommitTree 
        void (*MarkAborted)(TransactionId xid, int nxids, TransactionId *xids); // transam.c:TransactionIdAbortTree 

        XLogRecPtr (*GetCommitLSN)(TransactionId xid); // transam.c:TransactionIdGetCommitLSN
} TransactionManager;

// tqual.c
typedef struct 
{
        Snapshot (*AllocateSnapshot)(); // need to create snapshotы dynbamically 
        
        bool (*HeapTupleSatisfiesMVCC)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesSelf)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesDirty)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesUpdate)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesVacuum)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesAny)(HeapTuple htup, Snapshot snapshot, Buffer buffer);
        bool (*HeapTupleSatisfiesToast)(HeapTuple htup, Snapshot snapshot, Buffer buffer);

        bool (*IsInProgress)(TransactionId xid, Snaphot snapshot); // XidInMVCCSnapshot 
} SnapshotManager;
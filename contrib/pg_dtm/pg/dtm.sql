create type csn_t as bigint;

-- Extend global transaction to global. This function is called only once when coordimator decides to access 
-- data at some other node.
--
-- out: gcid_up (upper bound for gcid, see article)
create function dtm_extend() returns csn_t; 

-- Obtain new snapshot. This function is called by coordinator before executing new statement in read committed transaction.
-- Then coordinator should call dtm_access with this CSN at all nodes. 
--
-- out: gcid_up (upper bound for gcid, see article)
create function dtm_get_snapshot() returns csn_t; 

-- This function should be called by coordinator for all nodes pariticipated in global transaction except first node
-- (at which dtm_extend() is called.
--
-- in: gcid_up returned by dtm_extend
-- out: true if local transaction can be extended to global        
create function dtm_access(gcid_up csn_t) returns boolen; 

-- Get global transaction identifer:L actually CSN, assigned by DTM
-- This function is called before two-phase commit at any node
-- CSN assigned by dtm_get_gid() is passed by coordinator as parameter to PREPARE TRANSACTION 'csn'
-- If transaction is preapred at all nodes, then coordinator executes COMMIT TRANSACTION 'csn' at all nodes
-- and "select dtm_global_commit(csn)" at any of them
-- otherwise coordinator executes ROLLBACK TRANSACTION 'csn' at all nodes and "select dtm_global_abort(csn)" at any of them-
--
-- out: CSN assigned to this transaction
create function dtm_get_gid() returns csn_t; 

-- This function is called by coordinator at any node when first stage of commit is successfully completed at all nodes
-- 
-- in: CSN assigned by dtm_get_gid
-- out: true if transaction is globally committed (response returned by DTM)
create function dtm_global_commit(gtid csn_t) returns boolean;

-- This function is called by coordinator at any node when first stage of commit is failed at some of nodes.
-- in: CSN assigned by dtm_get_gid
create function dtm_global_abort(gtid csn_t); 

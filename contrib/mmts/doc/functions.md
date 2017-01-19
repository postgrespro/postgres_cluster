# `Built-in functions and views`

## Cluster information functions

* `mtm.get_nodes_state()` — show status of nodes in cluster. Returns tuple of following values:
    * id, integer
    * disabled, bool
    * disconnected, bool
    * catchUp, bool
    * slotLag, bigint
    * avgTransDelay, bigint
    * lastStatusChange, timestamp
    * oldestSnapshot, bigint
    * SenderPid integer
    * SenderStartTime timestamp
    * ReceiverPid integer
    * ReceiverStartTime timestamp
    * connStr text
    * connectivityMask bigint

* `mtm.get_cluster_state()` -- show whole cluster status
    * status, text
    * disabledNodeMask, bigint
    * disconnectedNodeMask, bigint
    * catchUpNodeMask, bigint
    * liveNodes, integer
    * allNodes, integer
    * nActiveQueries, integer
    * nPendingQueries, integer
    * queueSize, bigint
    * transCount, bigint
    * timeShift, bigint
    * recoverySlot, integer
    * xidHashSize, bigint
    * gidHashSize, bigint
    * oldestXid, bigint
    * configChanges, integer


## Node management functions

* `mtm.add_node(conn_str text)` -- add node to the cluster.
* `mtm.drop_node(node integer, drop_slot bool default false)` -- exclude node from the cluster.
* `mtm.poll_node(nodeId integer, noWait boolean default FALSE)` -- wait for node to become online.
* `mtm.recover_node(node integer)` -- create replication slot for the node which was previously dropped together with it's slot.

## Data management functions

* `mtm.make_table_local(relation regclass)` -- stop replication for a given table

## Debug functions

* `mtm.get_cluster_info()` -- print some debug info
* `mtm.inject_2pc_error`
* `mtm.check_deadlock`
* `mtm.start_replication`
* `mtm.stop_replication`
* `mtm.get_snapshot`
* `mtm.get_csn`
* `mtm.get_trans_by_gid`
* `mtm.get_trans_by_xid`
* `mtm.get_last_csn`
* `mtm.dump_lock_graph`

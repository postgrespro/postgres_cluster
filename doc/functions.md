# `Built-in functions and views`

## Cluster information functions

* `mtm.get_nodes_state()` — Shows the status of all nodes in the cluster. 
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

* `mtm.get_cluster_state()` -- Shows the status of the whole cluster. 
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

* `mtm.add_node(conn_str text)` -- Adds a new node to the cluster.
    * `conn_str` - Connection string for the new node. For example, for the database `mydb`, user `myuser`, and the new node `node4`, the connection string is `"dbname=mydb user=myuser host=node4"`. Type: `text`


* `mtm.drop_node(node integer, drop_slot bool default false)` -- Excludes a node from the cluster.
    * `node` - ID of the node to be dropped that you specified in the `multimaster.node_id` variable. Type: `integer`
    * `drop_slot` - Optional. Defines whether the replication slot should be dropped together with the node. Set this option to true if you do not plan to restore the node in the future. Type: `boolean` Default: `false`


* `mtm.recover_node(node integer)` -- Creates a replication slot for the node that was previously dropped together with its slot.
    * `node` - ID of the node to be restored. 


* `mtm.poll_node(nodeId integer, noWait boolean default FALSE)` -- Waits for the node to become online.


## Data management functions

* `mtm.make_table_local(relation regclass)` -- Stops replication for the specified table.
    * `relation` - The table you would like to exclude from the replication scheme. Type: `regclass`

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

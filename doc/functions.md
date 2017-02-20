# `Built-in functions and views`

## Cluster information functions

* `mtm.get_nodes_state()` — Shows the status of nodes in the cluster. 
    * id, integer - node ID.
    * enabled, bool - shows whether node was excluded from cluster. Node can only be disabled due to missing responses to heartbeats during `heartbeat_recv_timeout`. When node will start responding to hearbeats it will be recovered and turned back to enabled state. Automatic recovery only possible when replication slot is active. Otherwise see section [manual node recovery].
    * connected, bool - shows that node connected to our walsender.
    * slot_active, bool - node has active replication slot. For a disabled node slot will be active until `max_recovery_lag` will be reached.
    * stopped, bool - shows whether replication to this node was stopped by `mtm.stop_node()` function. Stopped node acts as a disabled one, but will not be automatically recovered. Call `mtm.recover_node()` to enable it again.
    * catchUp - during recovery shows that node recovered till `min_recovery_lag`.
    * slotLag - amount of WALs that replication slot holds for disabled/stoped node. Slot will be dropped whne slotLag reaches `max_recovery_lag`.
    * avgTransDelay - average delay to transaction commit imposed by this node, usecs.
    * lastStatusChange - last time when node changed it's status (enabled/disabled).
    * oldestSnapshot - oldest global snapshot existing on this node.
    * SenderPid - pid of WALSender.
    * SenderStartTime - time when WALSender was started.
    * ReceiverPid - pid of WALReceiver.
    * ReceiverStartTime - time when WALReceiver was started.
    * connStr - connection string to this node.
    * connectivityMask - bitmask representing connectivity to neighbor nodes. Each bit means connection to node.
    * nHeartbeats - number of hearbeat responses received from that node.

* `mtm.collect_cluster_state()` - Collects output of `mtm.get_cluster_state()` from all available nodes. Note: for this function to work pg_hba should also allow ordinary connections (in addition to replication) to node with specified connstring.

* `mtm.get_cluster_state()` - Get info about interanal state of multimaster extension. 
    * status - Node status. Can be "Initialization", "Offline", "Connected", "Online", "Recovery", "Recovered", "InMinor", "OutOfService".
    * disabledNodeMask - bitmask of disabled nodes.
    * disconnectedNodeMask - bitmask of disconnected nodes.
    * catchUpNodeMask - bitmask of nodes that completed their recovery.
    * liveNodes - number of enabled nodes.
    * allNodes - number of all nodes added to cluster. Decisions about majority of alive nodes based on that parameter.
    * nActiveQueries - number of queries being currently processed on this node.
    * nPendingQueries - number of queries avaiting their turn on this node.
    * queueSize - size of pending queue in bytes.
    * transCount - total amount of replicated transactions processed by this node.
    * timeShift - global snapshot shit due to unsynchronized clocks on nodes, usec.
    * recoverySlot - during recovery procedure node grabs changes from this node.
    * xidHashSize - size of xid2state hash.
    * gidHashSize - size of gid2state hash.
    * oldestXid - oldest xid on this node.
    * configChanges - number of state changes (enabled/disabled) since last reboot.
    * stalledNodeMask - bitmask of nodes for which replication slot was dropped.
    * stoppedNodeMask - bitmask of nodes that were stopped by `mtm.stop_node()`.
    * lastStatusChange - timestamp when last state change happend.


## Node management functions

* `mtm.add_node(conn_str text)` -- Adds a new node to the cluster.
    * `conn_str` - Connection string for the new node. For example, for the database `mydb`, user `myuser`, and the new node `node4`, the connection string is `"dbname=mydb user=myuser host=node4"`. Type: `text`


* `mtm.stop_node(node integer, drop_slot bool default false)` -- Excludes a node from the cluster.
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

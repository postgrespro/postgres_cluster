# `Built-in functions and views`

## Cluster information functions

* `mtm.get_nodes_state()` — Shows the status of nodes in the cluster. Returns a tuple of the following values:
    * id, integer - Node ID.
    * enabled, bool - Shows whether the node is excluded from the cluster. The node can only be disabled if responses to heartbeats are not received within the `heartbeat_recv_timeout` time interval. When the node starts responding to heartbeats, `multimaster` can automatically restore the node and switch it back to the enabled state. Automatic recovery is only possible if the replication slot is still active. Otherwise, you can restore the node manually.
    * connected, bool - Shows whether the node is connected to the WAL sender.
    * slot_active, bool - Shows whether the node has an active replication slot. For a disabled node, the slot remains active until the `max_recovery_lag` value is reached.
    * stopped, bool - Shows whether replication to this node was stopped by the `mtm.stop_node()` function. A stopped node acts as a disabled one, but cannot be automatically recovered. Call `mtm.recover_node()` to re-enable such a node.
    * catchUp - During the node recovery, shows whether the data is recovered up to the `min_recovery_lag` value.
    * slotLag - Size of the WAL data that the replication slot holds for a disabled/stopped node. The slot is dropped when `slotLag` reaches the `max_recovery_lag` value.
    * avgTransDelay - An average commit delay caused by this node, in microseconds.
    * lastStatusChange - Last time when the node changed its status (enabled/disabled).
    * oldestSnapshot - The oldest global snapshot existing on this node.
    * SenderPid - Process ID of the WAL sender.
    * SenderStartTime - WAL sender start time.
    * ReceiverPid - Process ID of the WAL receiver.
    * ReceiverStartTime - WAL receiver start time.
    * connStr - Connection string to this node.
    * connectivityMask - Bitmask representing connectivity to neighbor nodes. Each bit represents a connection to node.
    * nHeartbeats - Number of heartbeat responses received from this node.

* `mtm.collect_cluster_state()` - Collects the data returned by the `mtm.get_cluster_state()` function from all available nodes. For this function to work, in addition to replication connections, pg_hba.conf must allow ordinary connections to the node with the specified connection string.

* `mtm.get_cluster_state()` - Shows the status of the multimaster extension. Returns a tuple of the following values:
    * status - Node status. Possible values are: "Initialization", "Offline", "Connected", "Online", "Recovery", "Recovered", "InMinor", "OutOfService". The <literal>inMinor</literal> status indicates that the corresponding node got disconnected from the majority of the cluster nodes. Even though the node is active, it will not accept write transactions until it is reconnected to the cluster.
    * disabledNodeMask - Bitmask of disabled nodes.
    * disconnectedNodeMask - Bitmask of disconnected nodes.
    * catchUpNodeMask - Bitmask of nodes that completed the recovery.
    * liveNodes - Number of enabled nodes.
    * allNodes - Number of nodes in the cluster. The majority of alive nodes is calculated based on this parameter.
    * nActiveQueries - Number of queries being currently processed on this node.
    * nPendingQueries - Number of queries waiting for execution on this node.
    * queueSize - Size of the pending query queue, in bytes.
    * transCount - The total number of replicated transactions processed by this node.
    * timeShift - Global snapshot shift caused by unsynchronized clocks on nodes, in microseconds.
    * recoverySlot - The node from which a failed node gets data updates during automatic recovery.
    * xidHashSize - Size of xid2state hash.
    * gidHashSize - Size of gid2state hash.
    * oldestXid - The oldest transaction ID on this node.
    * configChanges - Number of state changes (enabled/disabled) since the last reboot.
    * stalledNodeMask - Bitmask of nodes for which replication slots were dropped.
    * stoppedNodeMask - Bitmask of nodes that were stopped by `mtm.stop_node()`.
    * lastStatusChange - Timestamp of the last state change.


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

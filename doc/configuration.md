# `GUC Variables`

```multimaster.node_id``` Node ID - a unique natural number identifying the node of a multi-master cluster. You must start node numbering from 1 and cannot have any gaps in numbering. For example, for a cluster of five nodes, set node IDs to 1, 2, 3, 4, and 5. 

```multimaster.conn_strings``` Connection strings for each node of a multi-master cluster, separated by commas. Each connection string must include the name of the database to replicate and the cluster node domain name. For example, 'dbname=mydb host=node1, dbname=mydb host=node2, dbname=mydb host=node3'. Connection strings must appear in the order of the node IDs specified in the ```multimaster.node_id``` variable. Connection string for the i-th node must be on the i-th position. This parameter must be identical on all nodes. You can specify a custom port for all connection strings using the `multimaster.arbiter_port` variable. 

```multimaster.arbiter_port``` Port for the arbiter process to listen on. 
Default: 5433

```multimaster.heartbeat_send_timeout``` Time interval between heartbeat messages, in milliseconds. An arbiter process broadcasts heartbeat messages to all nodes to detect connection problems. Default: 1000.

```multimaster.heartbeat_recv_timeout``` Timeout, in milliseconds. If no heartbeat message is received from the node within this timeframe, the node is excluded from the cluster. 
Default: 10000


```multimaster.min_recovery_lag``` Minimal WAL lag between the current cluster state and the node to be restored, in bytes. When this threshold is reached during node recovery, the cluster is locked for write transactions until the recovery is complete. 
Default: 100000

```multimaster.max_recovery_lag``` Maximal WAL lag size, in bytes. When a node is disconnected from the cluster, other nodes copy WALs for all new trasactions into the replication slot of this node. Upon reaching the `multimaster.max_recovery_lag` value, the replication slot for the disconnected node is deleted to avoid overflow. At this point, automatic recovery of the node is no longer possible. In this case, you can restore the node manually by cloning the data from one of the alive nodes using `pg_basebackup` or a similar tool. If you set this variable to zero, replication slot will not be deleted. 
Default: 10000000

```multimaster.ignore_tables_without_pk``` Boolean. This variable enables/disables replication of tables without primary keys. By default, replication of tables without primary keys is disabled because of the logical replication restrictions. To enable replication, you can set this variable to false. However, take into account that `multimaster` does not allow update operations on such tables. Default: true

```multimaster.cluster_name``` Name of the cluster. If you set this variable, `multimaster` checks that the cluster name is the same for all the cluster nodes.



## Questionable

(probably we will delete that variables, most of them are useful only for development purposes --stas)

```multimaster.min_2pc_timeout``` Minimal timeout between receiving PREPARED message from nodes participated in transaction to coordinator (milliseconds). Default = 2000, /* 2 seconds */.

```multimaster.max_2pc_ratio``` Maximal ratio (in percents) between prepare time at different nodes: if T is time of preparing transaction at some node, then transaction can be aborted if prepared responce was not received in T*MtmMax2PCRatio/100. default = 200, /* 2 times */

```multimaster.queue_size``` Multimaster queue size. default = 256*1024*1024,

```multimaster.trans_spill_threshold``` Maximal size (Mb) of transaction after which transaction is written to the disk. Default = 1000, /* 1Gb */ (istm reorderbuffer also can do that, isn't it?)

```multimaster.vacuum_delay``` Minimal age of records which can be vacuumed (seconds). default = 1.

```multimaster.worker``` Number of multimaster executor workers. Default = 8. (use dynamic workers with some timeout to die?)

```multimaster.max_worker``` Maximal number of multimaster dynamic executor workers. (set this to max_conn?) Default = 100.

```multimaster.gc_period``` Number of distributed transactions after which garbage collection is started. Multimaster is building xid->csn hash map which has to be cleaned to avoid hash overflow. This parameter specifies interval of invoking garbage collector for this map. default = MTM_HASH_SIZE/10

```multimaster.node_disable_delay``` Minimal amount of time (msec) between node status change. This delay is used to avoid false detection of node failure and to prevent blinking of node status node. default = 2000. (We can just increase heartbeat_recv_timeout)

```multimaster.connect_timeout``` Multimaster nodes connect timeout. Interval in milliseconds for establishing connection with cluster node. default = 10000, /* 10 seconds */

```multimaster.reconnect_timeout``` Multimaster nodes reconnect timeout. Interval in milliseconds for establishing connection with cluster node. default = 5000, /* 5 seconds */

```multimaster.use_dtm``` Use distributed transaction manager.

```multimaster.preserve_commit_order``` Transactions from one node will be committed in same order al all nodes.

```multimaster.volkswagen_mode``` Pretend to be normal postgres. This means skip some NOTICE's and use local sequences. Default false.






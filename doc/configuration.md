# `Configuration parameters`

```multimaster.node_id``` Multimaster node ID, unique number identifying this node. Nodes should be numbered by natural numbers starting from 1 without gaps (e.g. 1, 2, 3, ...). node_id is also used as an offset in ```multimaster.conn_strings```, thus i-th node's connection string expected to be on i-th position in ```multimaster.conn_strings```. Mandatory.

```multimaster.conn_strings``` Multimaster node connection strings separated by commas, i.e. 'dbname=mydb host=node1, dbname=mydb host=node2, dbname=mydb host=node3'. Order here is important and should be consistent with ```multimaster.node_id```. Multimaster allows to specify custom arbiter_port value for all connection strings. Also this parameter is expected to be identical on all nodes. Mandatory.

```multimaster.arbiter_port``` Port for arbiter process to listen on. Default to 5433.

```multimaster.heartbeat_send_timeout``` Period of broadcasting heartbeat messages by arbiter to all nodes. In milliseconds. Default to 1000.

```multimaster.heartbeat_recv_timeout``` If no heartbeat message is received from node within this period, it assumed to be dead. In milliseconds. Default to 10000.

```multimaster.min_recovery_lag``` Minimal lag of WAL-sender performing recovery after which cluster is locked until recovery is completed. When wal-sender almost catch-up WAL current position we need to stop 'Achilles tortile competition' and temporary stop commit of new transactions until node will be completely repared. In bytes. Default to 100000.

```multimaster.max_recovery_lag``` Maximal lag of replication slot of failed node after which this slot is dropped to avoid transaction log overflow. Dropping slot makes it not possible to recover node using logical replication mechanism, it will be necessary to completely copy content of some alive node using pg_basebackup or similar tool. Zero value of parameter disable slot dropping. In bytes. Default to 100000000.

```multimaster.ignore_tables_without_pk``` Do not replicate tables withpout primary key. Boolean.


## Questinable

(probably we will delete that variables, most of them useful only for development purposes --stas)

```multimaster.cluster_name``` Name of the cluster, desn't affect anything. Just in case.

```multimaster.min_2pc_timeout``` Minimal timeout between receiving PREPARED message from nodes participated in transaction to coordinator (milliseconds). Default = 2000, /* 2 seconds */.

```multimaster.max_2pc_ratio``` Maximal ratio (in percents) between prepare time at different nodes: if T is time of preparing transaction at some node, then transaction can be aborted if prepared responce was not received in T*MtmMax2PCRatio/100. default = 200, /* 2 times */

```multimaster.queue_size``` Multimaster queue size. default = 256*1024*1024,

```multimaster.vacuum_delay``` Minimal age of records which can be vacuumed (seconds). default = 1.

```multimaster.worker``` Number of multimaster executor workers. Default = 8. (use dynamic workers with some timeout to die?)

```multimaster.max_worker``` Maximal number of multimaster dynamic executor workers. (set this to max_conn?) Default = 100.

```multimaster.gc_period```  Number of distributed transactions after which garbage collection is started. Multimaster is building xid->csn hash map which has to be cleaned to avoid hash overflow. This parameter specifies interval of invoking garbage collector for this map. default = MTM_HASH_SIZE/10

```multimaster.max_node``` Maximal number of cluster nodes. This parameters allows to add new nodes to the cluster, default value 0 restricts number of nodes to one specified in multimaster.conn_strings (May be just set that to 64 and allow user to add node when trey need without restart?) default = 0

```multimaster.trans_spill_threshold``` Maximal size (Mb) of transaction after which transaction is written to the disk. Default = 1000, /* 1Gb */ (istm reorderbuffer also can do that, isn't it?)

```multimaster.node_disable_delay``` Minimal amount of time (msec) between node status change. This delay is used to avoid false detection of node failure and to prevent blinking of node status node. default = 2000. (We can just increase heartbeat_recv_timeout)

```multimaster.connect_timeout``` Multimaster nodes connect timeout. Interval in milliseconds for establishing connection with cluster node. default = 10000, /* 10 seconds */

```multimaster.reconnect_timeout``` Multimaster nodes reconnect timeout. Interval in milliseconds for establishing connection with cluster node. default = 5000, /* 5 seconds */

```multimaster.use_dtm``` Use distributed transaction manager.

```multimaster.preserve_commit_order``` Transactions from one node will be committed in same order al all nodes.

```multimaster.volkswagen_mode``` Pretend to be normal postgres. This means skip some NOTICE's and use local sequences. Default false.






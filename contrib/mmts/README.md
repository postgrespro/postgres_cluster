# `mmts`

An implementation of synchronous **multi-master replication** based on **commit timestamps**.

## Usage

1. Install `contrib/raftable` and `contrib/mmts` on each instance.
1. Add these required options to the `postgresql.conf` of each instance in the cluster.

 ```sh
 max_prepared_transactions = 200 # should be > 0, because all
                                 # transactions are implicitly two-phase
 max_connections = 200
 max_worker_processes = 100 # at least (2 * n + p + 1)
                            # this figure is calculated as:
                            #   1 raftable worker
                            #   n-1 receiver
                            #   n-1 sender
                            #   1 mtm-sender
                            #   1 mtm-receiver
                            #   p workers in the pool
 max_parallel_degree = 0
 wal_level = logical # multimaster is build on top of
                     # logical replication and will not work otherwise
 max_wal_senders = 10 # at least the number of nodes
 wal_sender_timeout = 0
 default_transaction_isolation = 'repeatable read'
 max_replication_slots = 10 # at least the number of nodes
 shared_preload_libraries = 'raftable,multimaster'
 multimaster.workers = 10
 multimaster.queue_size = 10485760 # 10mb
 multimaster.node_id = 1 # the 1-based index of the node in the cluster
 multimaster.conn_strings = 'dbname=... host=....0.0.1 port=... raftport=..., ...'
                            # comma-separated list of connection strings
 multimaster.use_raftable = true
 multimaster.heartbeat_recv_timeout = 1000
 multimaster.heartbeat_send_timeout = 250
 multimaster.ignore_tables_without_pk = true
 multimaster.twopc_min_timeout = 2000
 ```
1. Allow replication in `pg_hba.conf`.

## Status functions

`create extension mmts;` to gain access to these functions:

* `mtm.get_nodes_state()` -- show status of nodes on cluster
* `mtm.get_cluster_state()` -- show whole cluster status
* `mtm.get_cluster_info()` -- print some debug info
* `mtm.make_table_local(relation regclass)` -- stop replication for a given table

## Testing

* `make -C contrib/mmts check` to run TAP-tests.
* `make -C contrib/mmts xcheck` to run blockade tests. The blockade tests require `docker`, `blockade`, and some other packages installed, see [requirements.txt](tests2/requirements.txt) for the list. You might also want to gain superuser privileges to run these tests successfully.

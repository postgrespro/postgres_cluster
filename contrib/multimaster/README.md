# `multimaster`

An implementation of synchronous **multi-master replication** based on **snapshot sharing**.

## Usage

1. Install `contrib/arbiter` and `contrib/multimaster` on each instance.
1. Add these required options to the `postgresql.conf` of each instance in the cluster.

 ```sh
 multimaster.workers = 8
 multimaster.queue_size = 10485760 # 10mb
 multimaster.local_xid_reserve = 100 # number of xids reserved for local transactions
 multimaster.buffer_size = 0 # sockhub buffer size, if 0, then direct connection will be used
 multimaster.arbiters = '127.0.0.1:5431,127.0.0.1:5430'
                        # comma-separated host:port pairs where arbiters reside
 multimaster.conn_strings = 'replication=database dbname=postgres ...'
                            # comma-separated list of connection strings
 multimaster.node_id = 1 # the 1-based index of the node in the cluster
 shared_preload_libraries = 'multimaster'
 max_connections = 200
 max_replication_slots = 10 # at least the number of nodes
 wal_level = logical # multimaster is build on top of
                     # logical replication and will not work otherwise
 max_worker_processes = 100 # at least (FIXME: need an estimation here)
 ```

## Testing

1. `cd contrib/multimaster`
1. Deploy the cluster somewhere. You can use `tests/daemons.go` or one of `tests/*.sh` for that.
1. `make -C tests`
1. `tests/dtmbench ...`. See `tests/run.sh` for an example.

# `Postgresql multi-master`

Multi-master is an extension and set of patches to a Postegres database, that turns Postgres into a 
synchronous shared-nothing cluster to provide OLTP scalability and high availability with automatic
disaster recovery.

## Features

* Cluster-wide transaction isolation
* Synchronous logical replication
* DDL Replication
* Fault tolerance
* Automatic node recovery

## Overview

Multi-master replicates same database to all nodes in cluster and allows writes to each node. Transaction
isolation is enforced cluster-wide, so in case of concurrent updates on different nodes database will use the
same conflict resolution rules (mvcc with repeatable read isolation level) as single node uses for concurrent
backends and always stays in consistent state. Any writing transaction will write to all nodes, hence increasing
commit latency for amount of time proportional to roundtrip between nodes nedded for synchronization. Read only
transactions and queries executed locally without measurable overhead. Replication mechanism itself based on
logical decoding and earlier version of pglogical extension provided for community by 2ndQuadrant team.

Cluster consisting of N nodes can continue to work while majority of initial nodes are alive and reachable by
other nodes. This is done by using 3 phase commit protocol and heartbeats for failure discovery. Node that is
brought back to cluster can be fast-forwaded to actual state automatically in case when transactions log still
exists since the time when node was excluded from cluster (this depends on checkpoint configuration in postgres).

Read more about internals on [architecture](/contrib/mmts/doc/architecture.md) page.



## Installation

Multi-master consist of patched version of postgres and extension mmts, that provides most of functionality, but
doesn't requiere changes to postgres core. To run multimaster one need to install postgres and several extensions
to all nodes in cluster.

### Sources

Ensure that following prerequisites are installed: 

for Debian based linux:

```sh
apt-get install -y git make gcc libreadline-dev bison flex zlib1g-dev
```

for RedHat based linux:

```sh
yum groupinstall 'Development Tools'
yum install git, automake, libtool, bison, flex readline-devel
```

After that everything is ready to install postgres along with extensions

```sh
git clone https://github.com/postgrespro/postgres_cluster.git
cd postgres_cluster
./configure && make && make -j 4 install
cd ../../contrib/mmts && make install
```

### Docker

Directory contrib/mmts also includes docker-compose.yml that is capable of building multi-master and starting 3 node cluster.

```sh
cd contrib/mmts
docker-compose up
```

### PgPro packages

After things go more stable we will release prebuilt packages for major platforms.

## Configuration

1. Add these required options to the `postgresql.conf` of each instance in the cluster.
 ```sh
 wal_level = logical        # multimaster is build on top of
                            # logical replication and will not work otherwise
 max_connections = 100
 max_prepared_transactions = 300 # all transactions are implicitly two-phase, so that's
                                 # a good idea to set this equal to max_connections*N_nodes.
 max_wal_senders = 10       # at least the number of nodes
 max_replication_slots = 10 # at least the number of nodes
 max_worker_processes = 250 # Each node has:
                            #   N_nodes-1 receiver
                            #   N_nodes-1 sender
                            #   1 mtm-sender
                            #   1 mtm-receiver
                            # Also transactions executed at neighbour nodes can cause spawn of
                            # background pool worker at our node. At max this will be equal to
                            # sum of max_connections on neighbour nodes.



 shared_preload_libraries = 'multimaster'
 multimaster.max_nodes = 3  # cluster size
 multimaster.node_id = 1    # the 1-based index of the node in the cluster
 multimaster.conn_strings = 'dbname=mydb host=node1.mycluster, ...'
                            # comma-separated list of connection strings to neighbour nodes.
```
2. Allow replication in `pg_hba.conf`.

Read description of all configuration params at [configuration](/contrib/mmts/doc/configuration.md)

## Management

`create extension mmts;` to gain access to these functions:

* `mtm.get_nodes_state()` -- show status of nodes on cluster
* `mtm.get_cluster_state()` -- show whole cluster status
* `mtm.get_cluster_info()` -- print some debug info
* `mtm.make_table_local(relation regclass)` -- stop replication for a given table

Read description of all management functions at [functions](/contrib/mmts/doc/functions.md)



## Tests

### Performance

(Show TPC-C here on 3 nodes)

### Fault tolerance

(Link to test/failure matrix)


## Limitations

* Commit latency.
Current implementation of logical replication sends data to subscriber nodes only after local commit, so in case of
heavy-write transaction user will wait for transaction processing two times: on local node and al other nodes
(simultaneosly). We have plans to address this issue in future.

* DDL replication.
While data is replicated on logical level, DDL replicated by statements performing distributed commit with the same
statement. Some complex DDL scenarious including stored procedures and temp temp tables aren't working properly. We
are working right now on proving full compatibility with ordinary postgres. Currently we are passing 141 of 164
postgres regression tests.

* Isolation level.
Multimaster currently support only _repeatable_ _read_ isolation level. This is stricter than default _read_ _commited_,
but also increases probability of serialization failure during commit. _Serializable_ level isn't supported yet.

* One database per cluster.



## Credits and Licence

Multi-master developed by the PostgresPro team.

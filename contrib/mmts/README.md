# `Postgresql multi-master`

Multi-master is an extension and set of patches to a Postegres database, that turns Postgres into a synchronous shared-nothing cluster to provide OLTP scalability and high availability with automatic disaster recovery.


## Features

* Cluster-wide transaction isolation
* Synchronous logical replication
* DDL Replication
* Fault tolerance
* Automatic node recovery


## Overview

Multi-master replicates same database to all nodes in cluster and allows writes to each node. Transaction isolation is enforced cluster-wide, so in case of concurrent updates on different nodes database will use the same conflict resolution rules (mvcc with repeatable read isolation level) as single node uses for concurrent backends and always stays in consistent state. Any writing transaction will write to all nodes, hence increasing commit latency for amount of time proportional to roundtrip between nodes nedded for synchronization. Read only transactions and queries executed locally without measurable overhead. Replication mechanism itself based on logical decoding and earlier version of pglogical extension provided for community by 2ndQuadrant team.

Cluster consisting of N nodes can continue to work while majority of initial nodes are alive and reachable by other nodes. This is done by using 3 phase commit protocol and heartbeats for failure discovery. Node that is brought back to cluster can be fast-forwaded to actual state automatically in case when transactions log still exists since the time when node was excluded from cluster (this depends on checkpoint configuration in postgres).


## Documentation

1. [Administration](doc/administration.md)
    1. [Installation](doc/administration.md)
    1. [Setting up empty cluster](doc/administration.md)
    1. [Setting up cluster from pre-existing database](doc/administration.md)
    1. [Tuning configuration params](doc/administration.md)
    1. [Monitoring](doc/administration.md)
    1. [Adding nodes to cluster](doc/administration.md)
    1. [Excluding nodes from cluster](doc/administration.md)
1. [Architecture and internals](doc/architecture.md)
1. [List of configuration variables](doc/configuration.md)
1. [Built-in functions and views](doc/configuration.md)


## Tests

### Fault tolerance

(Link to test/failure matrix)

### Performance

(Show TPC-C here on 3 nodes)


## Limitations

* Commit latency.
Current implementation of logical replication sends data to subscriber nodes only after local commit, so in case of heavy-write transaction user will wait for transaction processing two times: on local node and on all other nodes (simultaneosly). We have plans to address this issue in future.

* DDL replication.
While data is replicated on logical level, DDL replicated by statements performing distributed commit with the same statement. Some complex DDL scenarious including stored procedures and temp temp tables aren't working properly. We are working right now on proving full compatibility with ordinary postgres. Currently we are passing 141 of 164 postgres regression tests.

* Isolation level.
Multimaster currently support only _repeatable_ _read_ isolation level. This is stricter than default _read_ _commited_, but also increases probability of serialization failure during commit. _Serializable_ level isn't supported yet.

* One database per cluster.


## Credits and Licence

Multi-master developed by the PostgresPro team.

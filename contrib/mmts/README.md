# `PostgreSQL multimaster`

`multimaster` is a PostgreSQL extension with a set of patches that turns PostgreSQL into a synchronous shared-nothing cluster to provide Online Transaction Processing (OLTP) scalability and high availability with automatic disaster recovery. As compared to a standard PostgreSQL master-slave cluster, a cluster configured with the `multimaster` extension offers the following benefits:

* Cluster-wide transaction isolation
* Synchronous logical replication 
* DDL replication
* Working with temporary tables on each cluster node
* Fault tolerance and automatic node recovery
* PostgreSQL online upgrades

## Overview

The `multimaster` extension replicates the same database to all nodes of the cluster and allows write transactions on each node. To ensure data consistency in the case of concurrent updates, `multimaster` enforces transaction isolation cluster-wide, using multiversion concurrency control (MVCC) at the repeatable read isolation level. Any write transaction is synchronously replicated to all nodes, which increases commit latency for the time required for synchronization. Read-only transactions and queries are executed locally, without any measurable overhead.  

To ensure high availability and fault tolerance of the cluster, `multimaster` uses three-phase commit protocol and heartbeats for failure discovery. A multi-master cluster of N nodes can continue working while the majority of the nodes are alive and reachable by other nodes. When the node is reconnected to the cluster, `multimaster` can automatically fast-forward the node to the actual state based on the transactions log (WAL). If WAL is no longer available for the time when the node was excluded from the cluster, you can restore the node using `pg_basebackup`.

For details on the `multimaster` internals, see the [Architecture](/contrib/mmts/doc/architecture.md) page.

## Documentation

1. [Administration](doc/administration.md)
    1. [Installation](doc/administration.md#installation)
    1. [Setting up a Multi-Master Cluster](doc/administration.md#setting-up-a-multi-master-cluster)
    1. [Tuning configuration params](doc/administration.md#tuning-configuration-parameters)
    1. [Monitoring Cluster Status](doc/administration.md#monitoring-cluster-status)
    1. [Adding New Nodes to the Cluster](doc/administration.md#adding-new-nodes-to-the-cluster)
    1. [Excluding Nodes from the Cluster](doc/administration.md#excluding-nodes-from-the-cluster)
1. [Architecture](doc/architecture.md)
1. [Configuration Variables](doc/configuration.md)
1. [Built-in Functions and Views](doc/functions.md)


## Tests

### Fault tolerance

(Link to test/failure matrix)

### Performance

(Show TPC-C here on 3 nodes)


## Limitations

* `multimaster` can only replicate one database per cluster.

* The replicated tables must have primary keys or replica identity. Otherwise, `multimaster` cannot perform logical replication. Unlogged tables are not replicated, as in the standard PostgreSQL.

* Sequence generation. 
To avoid conflicts between unique identifiers on different nodes, `multimaster` modifies the default behavior of sequence generators. For each node, ID generation is started with the node number and is incremented by the number of nodes in each iteration. For example, in a three-node cluster, 1, 4, and 7 IDs are allocated to the objects written onto the first node, while 2, 5, and 8 IDs are reserved for the second node. 

* DDL replication.
While `multimaster` replicates data on the logical level, DDL is replicated on the statement level, which causes distributed commits of the same statement on different nodes. As a result, complex DDL scenarios, such as stored procedures and temporary tables, may work differently as compared to the standard PostgreSQL. 

* Commit latency.
The current implementation of logical replication sends data to subscriber nodes only after the local commit. In case of a heavy-write transaction, you have to wait for transaction processing twice: on the local node and on all the other nodes (simultaneously). 

* Isolation level.
The `multimaster` extenstion currently supports only the _repeatable_ _read_ isolation level. This is stricter than the default _read_ _commited_ level, but also increases probability of serialization failure during commit. _Serializable_ level is not supported yet.


## Compatibility 
The `multimaster` extension currently passes 162 of 166 postgres regression tests. We are working right now on proving full compatibility with the standard PostgreSQL. 

## Authors

Postgres Professional, Moscow, Russia. 

### Credits
The replication mechanism is based on logical decoding and an earlier version of the `pglogical` extension provided for community by the 2ndQuadrant team.

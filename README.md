# Postgres_cluster

[![Build Status](https://travis-ci.org/postgrespro/postgres_cluster.svg?branch=master)](https://travis-ci.org/postgrespro/postgres_cluster)

Various experiments with PostgreSQL clustering perfomed at PostgresPro.

This is mirror of postgres repo with several changes to the core and few extra extensions.

## Core changes:

* Transaction manager interface (eXtensible Transaction Manager, xtm). Generic interface to plug distributed transaction engines. More info at [[https://wiki.postgresql.org/wiki/DTM]] and [[http://www.postgresql.org/message-id/flat/F2766B97-555D-424F-B29F-E0CA0F6D1D74@postgrespro.ru]].
* Distributed deadlock detection API.
* Logical decoding of two-phase transactions.


## New extensions:

* pg_tsdtm. Coordinator-less transaction management by tracking commit timestamps.
* multimaster. Synchronous multi-master replication based on logical_decoding and pg_dtm.


## Changed extension:

* postgres_fdw. Added support of pg_tsdtm.

## Installing multimaster

1. Build and install postgres from this repo on all machines in cluster.
1. Install contrib/raftable and contrib/mmts extensions.
1. Right now we need clean postgres installation to spin up multimaster cluster.
1. Create required database inside postgres before enabling multimaster extension.
1. We are requiring following postgres configuration:
    1. 'max_prepared_transactions' > 0 -- in multimaster all writing transaction along with ddl are wrapped as two-phase transaction, so this number will limit maximum number of writing transactions in this cluster node.
    1. 'synchronous_commit - off' -- right now we do not support async commit. (one can enable it, but that will not bring desired effect)
    1. 'wal_level = logical' -- multimaster built on top of logical replication so this is mandatory.
    1. 'max_wal_senders' -- this should be at least number of nodes - 1
    1. 'max_replication_slots' -- this should be at least number of nodes - 1
    1. 'max_worker_processes' -- at least 2*N + P - 1, where N is number of nodes in cluster, P size of pool of workers(see below) (1 raftable, n-1 receiver, n-1 sender, mtm-sender, mtm-receiver, + number of pool worker).
    1. 'default_transaction_isolation = 'repeatable read'' -- multimaster isn't supporting default read commited level.
1. Also multimaster have following configuration parameters:
    1. 'multimaster.conn_strings' -- connstrings for all nodes in cluster, separated by comma.
    1. 'multimaster.node_id' -- id of current node, number starting from one.
    1. 'multimaster.workers' -- number of workers that can apply transactions from neighbouring nodes.
    1. 'multimaster.use_raftable = true' -- just set this to true. Deprecated.
    1. 'multimaster.queue_size = 52857600' -- queue size for applying transactions from neighbouring nodes. 
    1. 'multimaster.ignore_tables_without_pk = 1' -- do not replicate tables without primary key
    1. 'multimaster.heartbeat_send_timeout = 250' -- heartbeat period (ms).
    1. 'multimaster.heartbeat_recv_timeout = 1000' -- disconnect node if we miss heartbeats all that time (ms).
    1. 'multimaster.twopc_min_timeout = 40000' -- rollback stalled transaction after this period (ms).
    1. 'raftable.id'  -- id of current node, number starting from one.
    1. 'raftable.peers'  -- id of current node, number starting from one.


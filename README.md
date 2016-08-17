# postgres_cluster

[![Build Status](https://travis-ci.org/postgrespro/postgres_cluster.svg?branch=master)](https://travis-ci.org/postgrespro/postgres_cluster)

Various experiments with PostgreSQL clustering perfomed at PostgresPro.

This is a mirror of postgres repo with several changes to the core and a few extra extensions.

## Core changes:

* Transaction manager interface (eXtensible Transaction Manager, xtm). Generic interface to plug distributed transaction engines. More info on [postgres wiki](https://wiki.postgresql.org/wiki/DTM) and on [the email thread](http://www.postgresql.org/message-id/flat/F2766B97-555D-424F-B29F-E0CA0F6D1D74@postgrespro.ru).
* Distributed deadlock detection API.
* Logical decoding of transactions.

## New extensions:

### [`arbiter`](contrib/arbiter)
A distributed transaction management daemon.\
Used by `pg_dtm` and `multimaster`.

### [`mmts`](contrib/mmts)
A synchronous multi-master replication based on **logical decoding** and **xtm**.

### [`multimaster`](contrib/multimaster)
A synchronous multi-master replication based on **snapshot sharing**.

### [`pg_dtm`](contrib/pg_dtm)
A coordinator-based distributed transaction management implementation based on **snapshot sharing**.

### [`pg_tsdtm`](contrib/pg_tsdtm)
A coordinator-less distributed transaction management implementation based on **commit timestamps**.

### [`raftable`](contrib/raftable)
A key-value table replicated over Raft protocol.\
Used by `mmts`.

# postgres_cluster

[![Build Status](https://travis-ci.org/postgrespro/postgres_cluster.svg?branch=master)](https://travis-ci.org/postgrespro/postgres_cluster)

Various experiments with PostgreSQL clustering perfomed at PostgresPro.

This is a mirror of postgres repo with several changes to the core and a few extra extensions.

## Core changes:

* Transaction manager interface (eXtensible Transaction Manager, xtm). Generic interface to plug distributed transaction engines. More info on [postgres wiki](https://wiki.postgresql.org/wiki/DTM) and on [the email thread](http://www.postgresql.org/message-id/flat/F2766B97-555D-424F-B29F-E0CA0F6D1D74@postgrespro.ru).
* Distributed deadlock detection API.
* Logical decoding of transactions.

## New extensions:

The following table describes the features and the way they are implemented in our four main extensions:

|                            |commit timestamps             |snapshot sharing                    |
|---------------------------:|:----------------------------:|:----------------------------------:|
|**distributed transactions**|[`pg_tsdtm`](contrib/pg_tsdtm)|[`pg_dtm`](contrib/pg_dtm)          |
|**multimaster replication** |[`mmts`](contrib/mmts)        |[`multimaster`](contrib/multimaster)|

### [`mmts`](contrib/mmts)
An implementation of synchronous **multi-master replication** based on **commit timestamps**.

### [`multimaster`](contrib/multimaster)
An implementation of synchronous **multi-master replication** based on **snapshot sharing**.

### [`pg_dtm`](contrib/pg_dtm)
An implementation of **distributed transaction** management based on **snapshot sharing**.

### [`pg_tsdtm`](contrib/pg_tsdtm)
An implementation of **distributed transaction** management based on **commit timestamps**.

### [`arbiter`](contrib/arbiter)
A distributed transaction management daemon.
Used by `pg_dtm` and `multimaster`.

### [`raftable`](contrib/raftable)
A key-value table replicated over Raft protocol.
Used by `mmts`.

# `Multi-master architecture`

## Intro

Multi-master consists of two major subsystems: synchronous logical replication and hearbeat process that
respostible for helth check and cluster recovery.

## Replication

When postgres loads multi-master shared library it sets up logical replication producer an consumer to each node in the cluster and hooks into transaction commit pipeline. Since each server can accept writes it is possible that any server can abort transaction due to concurrent update - in the same way as it happens on a single server between different backends. Usual way of dealing with such situations is to perform transaction in two steps: first try to ensure that commit is possible (PREPARE stage) and if all nodes acknowledged that then we can finally commit. Postgres support such [[two-phase commit|https://www.postgresql.org/docs/9.6/static/sql-prepare-transaction.html]] procedure. So multi-master captures each commit statement and implicitly transforms it to PREPARE, waits when cohort (all nodes except our) will get that transaction via replication protocol and only after successfull responses from cohort finally commit it.

Also to be able to resist node crashes and network failures ordinary two-phase commit (2PC) is insufficient. When failure happens between PREPARE and COMMIT survived nodes may not have enough information to decide what to do with prepared transaction -- crashed node can already commit or abort that transaction, but didn't notified other nodes about that and such transaction will block resouces (hold locks) until recovery of crashed node. Otherwise if we decide to commit/abort transaction without knowing faled node's decision then we can end up with data inconsistencies in database when failed node will be recovered (e.g. failed node committed transaction but survived node aborted it).

To be able to deal with crashes E3PC commit protocol was used [1][2]. Main idea of 3PC-like protocols is to write intention to commit transaction before actual commit, introducing new message (PRECOMMIT) in protocol between PREPARE and COMMIT messages. That message is not used during normal work, but in case of failure all nodes have enough information to decide what to do with transaction using quorum-based voting procedure. For voting to complete protocol requires majority of nodes to be presenet, hence the rule that cluster of 2N+1 can tolerate N simultaneous failures.

This process summarized on following diagram:

![](mmts_commit.svg)

Here user, connected to a backend (BE) decides to commit his transaction. Multi-master extension hooks that commit and changes it to a PREPARE statement. During transaction execution walsender process (WS) already started to decode transaction to "reorder buffer", and by the time when PREPARE statement happend WS starting sending our transaction to all neighbouring nodes (cohort). Then cohort nodes applies that transaction in walreceiver process (WR) and, after succes, signaling arbbiter process (Arb on diagram, custom background worker implemented in multimaster) to send vote for transaction (prepared) on initiating node.
Arbiter process on initiating node wait until all nodes from cohort will send vote for transaction; after that he send "precommit" messages and waits till all nodes will respond to that with "precommited" message.
When all participating sites answered with "precommited" message arbiter signalling backend to stop waiting and commit our prepared transaction.
After that commit WAL record reaches cohort nodes via walsender/walreceiver connections.

[1] Idit Keidar, Danny Dolev. Increasing the Resilience of Distributed and Replicated Database Systems. http://dx.doi.org/10.1006/jcss.1998.1566

[2] Tim Kempster, Colin Stirling, Peter Thanisch. A more committed quorum-based three phase commit protocol. http://dx.doi.org/10.1007/BFb0056487


## DDL replication

## Failure detection and recovery




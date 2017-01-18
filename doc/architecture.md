# `Multi-master architecture`

## Intro

Multi-master consists of two major subsystems: synchronous logical replication and arbiter process that is
respostible for health checks and cluster recovery automation.

## Replication

When postgres loads multi-master shared library it sets up [[logical replication|logrep doc link]] producer an consumer to each node in the cluster and hooks into transaction commit pipeline. Since each server can accept writes it is possible that any server can abort transaction due to concurrent update - in the same way as it happens on a single server between different backends. Usual way of dealing with such situations is to perform transaction in two steps: first try to ensure that commit is possible (PREPARE stage) and if all nodes acknowledged that then we can finally commit. Postgres support such [[two-phase commit|https://www.postgresql.org/docs/9.6/static/sql-prepare-transaction.html]] procedure. So multi-master captures each commit statement and implicitly transforms it to PREPARE, waits when cohort (all nodes except our) will get that transaction via replication protocol and only after successfull responses from cohort finally commit it.

Also to be able to resist node crashes and network failures ordinary two-phase commit (2PC) is insufficient. When failure happens between PREPARE and COMMIT survived nodes may not have enough information to decide what to do with prepared transaction -- crashed node can already commit or abort that transaction, but didn't notified other nodes about that and such transaction will block resouces (hold locks) until recovery of crashed node. Otherwise if we decide to commit/abort transaction without knowing faled node's decision then we can end up with data inconsistencies in database when failed node will be recovered (e.g. failed node committed transaction but survived node aborted it).

To be able to deal with crashes E3PC commit protocol was used [1][2]. Main idea of 3PC-like protocols is to write intention to commit transaction before actual commit, introducing new message (PRECOMMIT) in protocol between PREPARE and COMMIT messages. That message is not used during normal work, but in case of failure all nodes have enough information to decide what to do with transaction using quorum-based voting procedure. For voting to complete protocol requires majority of nodes to be presenet, hence the rule that cluster of 2N+1 can tolerate N simultaneous failures.

This process summarized on following diagram:

![](https://cdn.rawgit.com/postgrespro/postgres_cluster/fac1e9fa/contrib/mmts/doc/mmts_commit.svg)

Here user, connected to a backend (BE) decides to commit his transaction. Multi-master extension hooks that commit and changes it to a PREPARE statement. During transaction execution walsender process (WS) already started to decode transaction to "reorder buffer", and by the time when PREPARE statement happend WS starting sending our transaction to all neighbouring nodes (cohort). Then cohort nodes applies that transaction in walreceiver process (WR) and, after succes, signaling arbbiter process (Arb on diagram, custom background worker implemented in multimaster) to send vote for transaction (prepared) on initiating node.
Arbiter process on initiating node wait until all nodes from cohort will send vote for transaction; after that he send "precommit" messages and waits till all nodes will respond to that with "precommited" message.
When all participating sites answered with "precommited" message arbiter signalling backend to stop waiting and commit our prepared transaction.
After that commit WAL record reaches cohort nodes via walsender/walreceiver connections.

[1] Idit Keidar, Danny Dolev. Increasing the Resilience of Distributed and Replicated Database Systems. http://dx.doi.org/10.1006/jcss.1998.1566

[2] Tim Kempster, Colin Stirling, Peter Thanisch. A more committed quorum-based three phase commit protocol. http://dx.doi.org/10.1007/BFb0056487


<!--

## DDL replication

Multi-master replicates such statements on statement-based level wrapping them as part of two-phase transaction.

## Sequences

-->

## Failure detection and recovery

While multi-master allows writes to each node it waits responses about transaction acknowledgement from all other nodes, so without special actions in case of failure of any node each commit will wait until failed node recovery. To deal with such kind of situations multi-master periodically send heartbeats to check health and connectivity between nodes. When several hearbeats to the node are lost in a row (see configuration parameters ```multimaster.heartbeat_recv_timeout``` and ```multimaster.heartbeat_send_timeout```) that node can be kicked out the cluster to allow writes to alive nodes.

For alive nodes there is no way to distinguish between faled node that stopped serving requests and network-partitioned node that isn't reacheable by other nodes, but can be reacheble by database users. So to protect from split-brain situations (conflicting writes to nodes in different network partitions) in case pf failure multi-master allow writes only to nodes that sees majority of other nodes. For example when 5-node multi-master cluster experienced failure that splitted network into two isolated subnets with 2 and 3 cluster nodes then multi-master based on heartbeats propagation info will continue to accept writes at each node in bigger patition and deny all writes in smaller one. Speking generaly cluster consisting from 2N+1 can tolerate N node failures and will be alive if any N+1 alive and connected to each other. In case of partial network split, when different nodes have different connectivity (for example in 3-node cluster when node B can't access node C, but node A can access both B and C) multi-master will find fully-connected subset of nodes and switch off other nodes. Each node maintance data structure that keeps status of all nodes from this node's point of view, that is accessible through ```mtm.get_nodes_state()``` system view.

When failed node connects back to the cluster recovery process is started. Recovering node will select one of the cluster nodes to apply changes that were made while node was offline. That process will continue till recovering catches up to ```multimaster.min_recovery_lag``` WAL lag (default: 100kB). After that all cluster locks for writes to allow recovery process to finish. After recovery is done returned node is promoted to online status and returned back to replication scheme as it was before failure. Such automatic recovery only possible when failed node WAL lag behind the working ones is not more then ```multimaster.max_recovery_lag```. When failed node's lag is bigger ```multimaster.max_recovery_lag``` then node should be manually recovered using pg_basebackup from one of the working nodes.

# `Multi-master architecture`

## Intro

Multi-master consists of two major subsystems: synchronous logical replication and an arbiter process that performs health checks and enables cluster recovery automation.

## Replication

Since each server can accept writes, any server can abort a transaction because of a concurrent update - in the same way as it happens on a single server between different backends. To ensure high availability and data consistency on all cluster nodes, `multimaster` uses [[logical replication|logrep doc link]] and three-phase E3PC commit protocol [1][2].  

When PostgeSQL loads the `multimaster` shared library, `multimaster` sets up a logical replication producer and consumer for each node, and hooks into the transaction commit pipeline. The typical data replication workflow consists of the following phases:

1. `PREPARE` phase: `multimaster` captures and implicitly transforms each `COMMIT` statement to a `PREPARE` statement. All the nodes that get the transaction via the replication protocol (*the cohort nodes*) send their vote for approving or declining the transaction to the arbiter process on the initiating node. This ensures that all the cohort can accept the transaction, and no write conflicts occur. For details on `PREPARE` transactions support in PostgreSQL, see the [PREPARE TRANSACTION](https://postgrespro.com/docs/postgresproee/9.6/sql-prepare-transaction) topic.
2. `PRECOMMIT` phase:  If all the cohort approves the transaction, the arbiter process sends a `PRECOMMIT` message to all the cohort nodes to express an intention to commit the transaction. The cohort nodes respond to the arbiter with the `PRECOMMITTED` message. In case of a failure, all the nodes can use this information to complete the transaction using a quorum-based voting procedure. 
3. `COMMIT` phase: If `PRECOMMIT` is successful, the arbiter commits the transaction to all nodes. 

If a node crashes or gets disconnected from the cluster between the `PREPARE` and `COMMIT` phases, the `PRECOMMIT` phase ensures that the survived nodes have enough information to complete the prepared transaction. The `PRECOMMITTED` messages help you avoid the situation when the crashed node have already committed or aborted the transaction, but have not notified other nodes about the transaction status. In a two-phase commit (2PC), such a transaction would block resources (hold locks) until the recovery of the crashed node. Otherwise, you could get data inconsistencies in the database when the failed node is recovered. For example, if the failed node committed the transaction but the survived node aborted it.

To complete the transaction, the arbiter must receive a response from the majority of the nodes. For example, for a cluster of 2N + 1 nodes, at least N+1 responses are required to complete the transaction. Thus, `multimaster` ensures that your cluster is available for reads and writes while the majority of the nodes are connected, and no data inconsistencies occur in case of a node or connection failure. 
For details on the failure detection mechanism, see [Failure Detection and Recovery](#failure-detection-and-recovery). 

This replication process is illustrated in the following diagram:

![](https://cdn.rawgit.com/postgrespro/postgres_cluster/fac1e9fa/contrib/mmts/doc/mmts_commit.svg)

1. When a user connected to a backend (**BE**) decides to commit a transaction, `multimaster` hooks the `COMMIT` and changes the `COMMIT` statement to a `PREPARE` statement. 
2. During the transaction execution, the `walsender` process (**WS**) starts to decode the transaction to "reorder buffer". By the time the `PREPARE` statement happens, **WS** starts sending the transaction to all the cohort nodes. 
3. The cohort nodes apply the transaction in the walreceiver process (**WR**). After success, votes for prepared transaction are sent to the initiating node by the arbiter process (**Arb**) - a custom background worker implemented in `multimaster`.
4. The arbiter process on the initiating node waits for all the cohort nodes to send their votes for transaction. After that, the arbiter sends `PRECOMMIT` messages and waits until all the nodes respond with the `PRECOMMITTED` message.
5. When all participating nodes answered with the `PRECOMMITTED` message, the arbiter signals the backend to stop waiting and commit the prepared transaction.
6. The `walsender`/`walreceiver` connections transmit the commit WAL records to the cohort nodes.

[1] Idit Keidar, Danny Dolev. Increasing the Resilience of Distributed and Replicated Database Systems. http://dx.doi.org/10.1006/jcss.1998.1566

[2] Tim Kempster, Colin Stirling, Peter Thanisch. A more committed quorum-based three phase commit protocol. http://dx.doi.org/10.1007/BFb0056487


<!--

## DDL replication

Multi-master replicates such statements on statement-based level wrapping them as part of two-phase transaction.

## Sequences

-->

## Failure Detection and Recovery

Since `multimaster` allows writes to each node, it has to wait for responses about transaction acknowledgement from all the other nodes. Without special actions in case of a node failure, each commit would have to wait until the failed node recovery. To deal with such situations, `multimaster` periodically sends heartbeats to check the node status and connectivity between nodes. When several heartbeats to the node are lost in a row, this node is kicked out of the cluster to allow writes to the remaining alive nodes. You can configure the heartbeat frequency and the response timeout in the ```multimaster.heartbeat_send_timeout``` and ```multimaster.heartbeat_recv_timeout``` parameters, respectively. 

For alive nodes, there is no way to distinguish between a failed node that stopped serving requests and a network-partitioned node that can be accessed by database users, but is unreachable for other nodes. To avoid conflicting writes to nodes in different network partitions, `multimaster` only allows writes to the nodes that see the majority of other nodes. 

For example, suppose a five-node multi-master cluster experienced a network failure that split the network into two isolated subnets, with two and three cluster nodes. Based on heartbeats propagation information, `multimaster` will continue to accept writes at each node in the bigger partition, and deny all writes in the smaller one. Thus, a cluster consisting of 2N+1 nodes can tolerate N node failures and stay alive if any N+1 nodes are alive and connected to each other. 

In case of a partial network split when different nodes have different connectivity, `multimaster` finds a fully connected subset of nodes and switches off other nodes. For example, in a three-node cluster, if node A can access both B and C, but node B cannot access node C, `multimaster` isolates node C to ensure data consistency on nodes A and B.  

Each node maintains a data structure that keeps the status of all nodes in relation to this node. You can get this status in the ```mtm.get_nodes_state()``` system view.

When a failed node connects back to the cluster, `multimaster` starts the automatic recovery process: 

1. The reconnected node selects a random cluster node and starts catching up with the current state of the cluster based on the Write-Ahead Log (WAL).  
2. When the node gets synchronized up to the minimum recovery lag, all cluster nodes get locked for writes to allow the recovery process to finish. By default, the minimum recovery lag is 100kB. You can change this value in the ```multimaster.min_recovery_lag``` variable. 
3. When the recovery is complete, `multimaster` promotes the reconnected node to the online status and includes it into the replication scheme. 

Automatic recovery is only possible if the failed node WAL lag behind the working ones does not exceed the ```multimaster.max_recovery_lag``` value. When the WAL lag is bigger than ```multimaster.max_recovery_lag```, you can manually restore the node from one of the working nodes using `pg_basebackup`.

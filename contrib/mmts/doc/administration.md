# `Administration`

1. [Installation](#installation)
1. [Setting up a Multi-Master Cluster](#setting-up-a-multi-master-cluster)
1. [Tuning Configuration Parameterss](#tuning-configuration-parameters)
1. [Monitoring Cluster Status](#monitoring-cluster-status)
1. [Adding New Nodes to the Cluster](#adding-nodes-to-cluster)
1. [Excluding Nodes from the Cluster](#excluding-nodes-from-cluster)



## Installation

Multi-master consists of a patched version of PostgreSQL and the `mmts` extension that provide most of the functionality, but depend on several modifications to PostgreSQL core.


### Building multimaster from Source Code

1. Depending on your operating system, ensure that the following prerequisites are installed.

For Debian based Linux:

```
apt-get install -y git make gcc libreadline-dev bison flex zlib1g-dev
```

For Red Hat based Linux:

```
yum groupinstall 'Development Tools'
yum install git, automake, libtool, bison, flex readline-devel
```

For macOS systems:
Make sure you have XCode command-line tools installed.

2. Install PostgreSQL with the multimaster extension:

```
git clone https://github.com/postgrespro/postgres_cluster.git
cd postgres_cluster
./configure --prefix=/path/to/install && make -j 4 install
cd contrib/mmts && make install
```

In this command, ```./configure``` is a standard PostgreSQL autotools script, so you can specify [any options](https://www.postgresql.org/docs/9.6/static/install-procedure.html) available in PostgreSQL. Make sure that /path/to/install/bin is specified in the ```PATH``` environment variable for the current user:

```
    export PATH=$PATH:/path/to/install/bin
```


### Docker

The contrib/mmts directory also includes docker-compose.yml that is capable of building `multimaster` and starting a three-node cluster.

First of all we need to build PGPro EE docker image (We will remove this step when we'll merge _MULTIMASTER branch and start building packages). In the repo root, run:
```
docker build -t pgproent
```

Then following command will start a cluster of 3 docker nodes listening on ports 15432, 15433 and 15434 (edit docker-compose.yml to change start params):
```
cd contrib/mmts
docker-compose up
```

### PgPro packages

To use `multimaster`, you need to install Postgres Pro Enterprise on all nodes of your cluster. Postgres Pro Enterprise includes all the required dependencies and extensions.


## Setting up a Multi-Master Cluster

After installing Postgres Pro Enterprise on all nodes, you need to configure the cluster with `multimaster`. Suppose you are setting up a cluster of three nodes, with ```node1```, ```node2```, and ```node3``` domain names.
To configure your cluster with `multimaster`, complete these steps on each cluster node:

1. Set up the database to be replicated with `multimaster`:

    * If you are starting from scratch, initialize a cluster, create an empty database `mydb` and a new user `myuser`, as usual:
    ```
    initdb -D ./datadir
    pg_ctl -D ./datadir -l ./pg.log start
    createdb myuser -h localhost
    createdb mydb -O myuser -h localhost
    pg_ctl -D ./datadir -l ./pg.log stop
    ```

    * If you already have a database `mydb` running on the `node1` server, initialize new nodes from the working node using `pg_basebackup`. On each cluster node you are going to add, run:
    ```
    pg_basebackup -D ./datadir -h node1 mydb
    ```
    For details, on `pg_basebackup`, see [pg_basebackup](https://www.postgresql.org/docs/9.6/static/app-pgbasebackup.html).

1. Modify the ```postgresql.conf``` configuration file, as follows:
    * Set up PostgreSQL parameters related to replication.

        ```
        wal_level = logical
        max_connections = 100
        max_prepared_transactions = 300
        max_wal_senders = 10       # at least the number of nodes
        max_replication_slots = 10 # at least the number of nodes
        ```
        You must change the replication level to `logical` as `multimaster` relies on logical replication. For a cluster of N nodes, enable at least N WAL sender processes and replication slots. Since `multimaster` implicitly adds a `PREPARE` phase to each `COMMIT` transaction, make sure to set the number of prepared transactions to N*max_connections. Otherwise, prepared transactions may be queued.

    * Make sure you have enough background workers allocated for each node:

        ```
        max_worker_processes = 250
        ```
        For example, for a three-node cluster with `max_connections` = 100, `multimaster` may need up to 206 background workers at peak times: 200 workers for connections from the neighbor nodes, two workers for walsender processes, two workers for walreceiver processes, and two workers for the arbiter sender and receiver processes. When setting this parameter, remember that other modules may also use backround workers at the same time.

    * Add `multimaster`-specific options:

        ```postgres
        multimaster.max_nodes = 3  # cluster size
        multimaster.node_id = 1    # the 1-based index of the node in the cluster
        multimaster.conn_strings = 'dbname=mydb user=myuser host=node1, dbname=mydb user=myuser host=node2, dbname=mydb user=myuser host=node3'
                                # comma-separated list of connection strings to neighbor nodes
        ```

        > **Important:** The `node_id` variable takes natural numbers starting from 1, without any gaps in numbering. For example, for a cluster of five nodes, set node IDs to 1, 2, 3, 4, and 5. In the `conn_strings` variable, make sure to list the nodes in the order of their IDs. The `conn_strings` variable must be the same on all nodes.

    Depending on your network environment and usage patterns, you may want to tune other `multimaster` parameters. For details on all configuration parameters available, see [Tuning Configuration Parameters](#tuning-configuration-parameters).

1. Allow replication in `pg_hba.conf`:

    ```
    host myuser all node1 trust
    host myuser all node2 trust
    host myuser all node3 trust
    host replication all node1 trust
    host replication all node2 trust
    host replication all node3 trust
    ```

1. Start PostgreSQL:

    ```
    pg_ctl -D ./datadir -l ./pg.log start
    ```

1. When PostgreSQL is started on all nodes, connect to any node and create the `multimaster` extension:
    ```
    psql -h node1
    > CREATE EXTENSION multimaster;
    ```

To ensure that `multimaster` is enabled, check the ```mtm.get_cluster_state()``` view:
```
> select * from mtm.get_cluster_state();
```
If `liveNodes` is equal to `allNodes`, you cluster is successfully configured and ready to use.
See Also
[Tuning Configuration Parameters](#tuning-configuration-parameters)


While you can use `multimaster` with the default configuration, you may want to tune several parameters for faster failure detection or more reliable automatic recovery.

### Setting Timeout for Failure Detection
To check availability of neighbour nodes, `multimaster` periodically sends heartbeat packets to all nodes: 

* The ```multimaster.heartbeat_send_timeout``` variable defines the time interval between sending the heartbeats. By default, this variable is set to 1000ms. 
* The ```multimaster.heartbeat_recv_timeout``` variable sets the timeout after which If no hearbeats were received during this time, the node is assumed to be disconnected and is excluded from the cluster. By default, this variable is set to 10000 ms. 

It's good idea to set ```multimaster.heartbeat_send_timeout``` based on typical ping latencies between you nodes. Small recv/send ratio decreases the time of failure detection, but increases the probability of false-positive failure detection. When setting this parameter, take into account the typical packet loss ratio between your cluster nodes.

### Configuring Automatic Recovery Parameters

If one of your node fails, `multimaster` can automatically restore the node based on the WAL logs collected on other cluster nodes. To control the recovery, use the following variables:

* ```multimaster.max_recovery_lag``` - sets the maximum size of WAL logs. Upon reaching the ```multimaster.max_recovery_lag``` threshold, WAL logs for the disconnected node are deleted. At this point, automatic recovery is no longer possible. If you need to restore the disconnected node, you have to clone it manually from one of the alive nodes using ```pg_basebackup```.
* ```multimaster.min_recovery_lag``` - sets the difference between the acceptor and donor nodes. When the disconnected node is fast-forwarded up to the ```multimaster.min_recovery_lag``` threshold, `multimaster` stops all new commits to the alive nodes to allow the node to fully catch up with the rest of the cluster. When the data is fully synchronized, the disconnected node is promoted to the online state, and the cluster resumes its work.

By default, ```multimaster.max_recovery_lag``` is set to 1GB. Setting  ```multimaster.max_recovery_lag``` to a larger value increases the timeframe for automatic recovery, but requires more disk space for WAL collection. 

## Monitoring Cluster Status

`multimaster` provides several views to check the current cluster state. 


To check node-specific information, use the ```mtm.get_nodes_state()```:

    ```sql
    select * from mtm.get_nodes_state();
    ```

To check the status of the whole cluster, use the ```mtm.get_cluster_state()``` view:

    ```sql
    select * from mtm.get_cluster_state();
    ```

For details on all the returned information, see [functions](doc/functions.md)


## Adding New Nodes to the Cluster

With mulmimaster, you can add or drop cluster nodes without restart. To add a new node, you need to change cluster configuration on alive nodes, load data to the new node using ```pg_basebackup```, and start the node.

Suppose we have a working cluster of three nodes, with ```node1```, ```node2```, and ```node3``` domain names. To add ```node4```, follow these steps:

1. Figure out the required connection string that will be used to access the new node. For example, for the database `mydb`, user `myuser`, and the new node `node4`, the connection string is "dbname=mydb user=myuser host=node4". 

1. In `psql` connected to any live node, run:

    ```sql
    select * from mtm.add_node('dbname=mydb user=myuser host=node4');
    ```

    This command changes the cluster configuration on all nodes and starts replication slots for a new node.

1. Copy all data from one of the alive nodes to the new node:

    ```
    node4> pg_basebackup -D ./datadir -h node1 -x
    ```

    ```pg_basebackup``` copies the entire data directory from ```node1```, together with configuration settings. 

1. Update ```postgresql.conf``` settings on ```node4```:

    ```
    multimaster.max_nodes = 4
    multimaster.node_id = 4
    multimaster.conn_strings = 'dbname=mydb user=myuser host=node1, dbname=mydb user=myuser host=node2, dbname=mydb user=myuser host=node3, dbname=mydb user=myuser host=node4'
    ```

1. Start PostgreSQL on the new node:

    ```
    node4> pg_ctl -D ./datadir -l ./pg.log start
    ```

    When switched on, the node recovers the recent transaction and changes its state to `online`. Now the cluster is using the new node. 

1. Update configuration settings on all cluster nodes to ensure that the right configuration is loaded in case of PostgreSQL restart:
* Change ```multimaster.conn_strings``` and ```multimaster.max_nodes``` on old nodes
* Make sure the `pg_hba.conf` files allows replication to the new node.

**See Also**
[Setting up a Multi-Master Cluster](#setting-up-a-multi-master-cluster)
[Monitoring Cluster Status](#monitoring-cluster-status)


## Excluding Nodes from the Cluster

To exclude a node from the cluster, use the `mtm.stop_node()` function. For example, to exclude node 3, run the following command on any other node:

    ```
    select mtm.stop_node(3);
    ```

This disables node 3 on all cluster nodes and stops replication to this node.

In general, if you simply shutdown a node, it will be excluded from the cluster as well. However, all transactions in the cluster will be frozen until other nodes detect the node offline status. This timeframe is defined by the ```multimaster.heartbeat_recv_timeout``` parameter. 





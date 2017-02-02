# `Administration`

1. [Installation](#installation)
1. [Setting up empty cluster](#setting-up-empty-cluster)
1. [Setting up cluster from pre-existing database](#setting-up-cluster-from-pre-existing-database)
1. [Tuning configuration params](#tuning-configuration-params)
1. [Monitoring](#monitoring)
1. [Adding nodes to cluster](#adding-nodes-to-cluster)
1. [Excluding nodes from cluster](#excluding-nodes-from-cluster)



## Installation

Multi-master consist of patched version of postgres and extension mmts, that provides most of the functionality, but depends on several modifications to postgres core.


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

Directory contrib/mmts also includes docker-compose.yml that is capable of building multi-master and starting 3 node cluster.

First of all we need to build PGPro EE docker image (We will remove this step when we'll merge _MULTIMASTER branch and start building packages). In the repo root run:
```
docker build -t pgproent
```

Then following command will start cluster of 3 docker nodes listening on port 15432, 15433 and 15434 (edit docker-compose.yml to change start params):
```
cd contrib/mmts
docker-compose up
```

### PgPro packages

PostgresPro Enterprise have all necessary dependencies and extensions included, so it is enough just install packages.


## Setting up empty cluster

After installing software on all cluster nodes we can configure our cluster. Here we describe how to set up multimaster consisting of 3 nodes with empty database. Suppose our nodes accesible via domain names ```node1```, ```node2``` and ```node3```. Perform following steps on each node (sequentially or in parallel – doesn't matter):

1. As with usual postgres first of all we need to initialize directiory where postgres will store it files:
    ```
    initdb -D ./datadir
    ```
    In that directory we are interested in files ```postgresql.conf``` and ```pg_hba.conf``` that are responsible for a general and security configuration consequently.

1. Create database that will be used with multimaster. This will require intermediate launch of postgres.

    ```
    pg_ctl -D ./datadir -l ./pg.log start
    createdb myuser -h localhost
    createdb mydb -O myuser -h localhost
    pg_ctl -D ./datadir -l ./pg.log stop
    ```

1. Modify the ```postgresql.conf``` configuration file, as follows:

    * Set up PostgreSQL parameters related to replication.

        ```
        wal_level = logical
        max_connections = 100
        max_prepared_transactions = 300
        max_wal_senders = 10       # at least the number of nodes
        max_replication_slots = 10 # at least the number of nodes
        ```
        You must change the replication level to `logical` as multimaster relies on logical replication. For a cluster of N nodes,and enable at least N WAL sender processes and replication slots. Since multimaster implicitly adds a PREPARE phase to each COMMIT transaction, make sure to set the number of prepared transactions to N*max_connections. Otherwise, prepared transactions may be queued.

    * Make sure you have enough background workers allocated for each node:

        ```
        max_worker_processes = 250
        ```
        For example, for a three-node cluster with max_connections = 100, multimaster may need up to 206 background workers at peak times: 200 workers for connections from the neighbour nodes, two workers for walsender processes, two workers for walreceiver processes, and two workers for the arbiter wender and receiver processes. When setting this parameter, remember that other modules may also use backround workers at the same time.

    * Add multimaster-specific options:

        ```
        multimaster.max_nodes = 3  # cluster size
        multimaster.node_id = 1    # the 1-based index of the node in the cluster
        multimaster.conn_strings = 'dbname=mydb user=myuser host=node1, dbname=mydb user=myuser host=node2, dbname=mydb user=myuser host=node3'
                                # comma-separated list of connection strings to neighbour nodes
        ```

        > **Important:** The `node_id` variable takes natural numbers starting from 1, without any gaps in numbering. For example, for a cluster of five nodes, set node IDs to 1, 2, 3, 4, and 5. In the `conn_strings` variable, make sure to list the nodes in the order of their IDs. Thus, the `conn_strings` variable must be the same on all nodes.

    Depending on your network environment and usage patterns, you may want to tune other multimaster parameters. For details on all configuration parameters available, see [Tuning configuration params](#tuning-configuration-params).

1. Allow replication in `pg_hba.conf`:

    ```
    host myuser all node1 trust
    host myuser all node2 trust
    host myuser all node3 trust
    host replication all node1 trust
    host replication all node2 trust
    host replication all node3 trust
    ```

1. Finally start postgres:

    ```
    pg_ctl -D ./datadir -l ./pg.log start
    ```

1. When postgres is started on all nodes you can connect to any node and create multimaster extention to get acces to monitoring functions:
    ```
    psql -h node1
    > CREATE EXTENSION multimaster;
    ```

    To enshure that everything is working check multimaster view ```mtm.get_cluster_state()```:

    ```
    > select * from mtm.get_cluster_state();
    ```

    Check that liveNodes in this view is equal to allNodes.


## Setting up cluster from pre-existing database

In case of preexisting database setup would be slightly different. Suppose we have running database on server ```node1``` and wan't to turn it to a multimaster by adding two new nodes ```node2``` and ```node3```. Instead of initializing new directory and creating database and user through a temporary launch, one need to initialize new node through pg_basebackup from working node.

1. On each new node run:

    ```
    pg_basebackup -D ./datadir -h node1 mydb
    ```

After that configure and atart multimaster from step 3 of previous section. See deteailed description of pg_basebackup options in the official [documentation](https://www.postgresql.org/docs/9.6/static/app-pgbasebackup.html).


## Tuning configuration params

While multimaster is usable with default configuration, several params may require tuning.

* Hearbeat timeouts — multimaster periodically send heartbeat packets to check availability of neighbour nodes. ```multimaster.heartbeat_send_timeout``` defines amount of time between sending heartbeats, while ```multimaster.heartbeat_recv_timeout``` sets amount of time following which node assumed to be disconnected if no hearbeats were received during this time. It's good idea to set ```multimaster.heartbeat_send_timeout``` based on typical ping latencies between you nodes. Small recv/senv ratio decreases time of failure detection, but increases probability of false positive failure detection, so tupical packet loss ratio between nodes should be taken into account.

* Min/max recovery lag — when node is disconnected from the cluster other nodes will keep to collect WAL logs for disconnected node until size of WAL log will grow to ```multimaster.max_recovery_lag```. Upon reaching this threshold WAL logs for disconnected node will be deleted, automatic recovery will be no longer possible and disconnected node should be cloned manually from one of alive nodes by ```pg_basebackup```. Increasing ```multimaster.max_recovery_lag``` increases amount of time while automatic recovery is possible, but also increasing maximum disk usage during WAL collection. On the other hand ```multimaster.min_recovery_lag``` sets difference between acceptor and donor nodes before switching ordanary recovery to exclusive mode, when commits on donor node are stopped. This step is necessary to ensure that no new commits will happend during node promotion from recovery state to online state and nodes will be at sync after that.


## Monitoring

Multimaster provides several views to check current cluster state. To access this functions ```multimaster``` extension should be created explicitely. Run in psql:

    ```sql
    CREATE EXTENSION multimaster;
    ```

Then it is possible to check nodes specific information via ```mtm.get_nodes_state()```:

    ```sql
    select * from mtm.get_nodes_state();
    ```

and status of whole cluster can bee seen through:

    ```sql
    select * from mtm.get_cluster_state();
    ```

Read description of all monitoring functions at [functions](doc/functions.md)

## Adding nodes to cluster

Mulmimaster is able to add/drop cluster nodes without restart. To add new node one should change cluster configuration on alive nodes, than load data to a new node using ```pg_basebackup``` and start node.

Suppose we have working cluster of three nodes (```node1```, ```node2```, ```node3```) and want to add new ```node4``` to the cluster.

1. First we need to figure out connection string that will be used to access new server. Let's assume that in our case that will be "dbname=mydb user=myuser host=node4". Run in psql connected to any live node:

    ```sql
    select * from mtm.add_node('dbname=mydb user=myuser host=node4');
    ```

    this will change cluster configuration on all nodes and start replication slots for a new node.

1. After calling ```mtm.add_node()``` we can copy data from alive node on new node:

    ```
    node4> pg_basebackup -D ./datadir -h node1 -x
    ```

1. ```pg_basebackup``` will copy entire data directory from ```node1``` among with configs. So we need to change ```postgresql.conf``` for ```node4```:

    ```
    multimaster.max_nodes = 4
    multimaster.node_id = 4
    multimaster.conn_strings = 'dbname=mydb user=myuser host=node1, dbname=mydb user=myuser host=node2, dbname=mydb user=myuser host=node3, dbname=mydb user=myuser host=node4'
    ```

1. Now we can just start postgres on new node:

    ```
    node4> pg_ctl -D ./datadir -l ./pg.log start
    ```

    After switching on node will recover recent transaction and change state to ONLINE. Node status can be checked via ```mtm.get_nodes_state()``` view on any cluster node.

1. Now cluster is using new node, but we also should change ```multimaster.conn_strings``` and ```multimaster.max_nodes``` on old nodes to ensure that right configuration will be loaded in case of postgres restart.


## Excluding nodes from cluster

Generally it is okay to just shutdown cluster node but all transaction in cluster will be freezed while other nodes will detect that node is gone (this period is controlled by the ```multimaster.heartbeat_recv_timeout``` parameter). To avoid such situation it is possible to exclude node from cluster in advance. 

For example to stop node 3 run on any other node:

    ```
    select mtm.stop_node(3);
    ```

This will disable node 3 on all cluster nodes.




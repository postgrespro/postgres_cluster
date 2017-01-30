# `Administration`

1. [Installation](doc/administration.md)
1. [Setting up empty cluster](doc/administration.md)
1. [Setting up cluster from pre-existing database](doc/administration.md)
1. [Tuning configuration params](doc/administration.md)
1. [Monitoring](doc/administration.md)
1. [Adding nodes to cluster](doc/administration.md)
1. [Excluding nodes from cluster](doc/administration.md)



## Installation

Multi-master consist of patched version of postgres and extension mmts, that provides most of the functionality, but depends on several modifications to postgres core.


### Sources

Ensure that following prerequisites are installed: 

for Debian based linux:

```
apt-get install -y git make gcc libreadline-dev bison flex zlib1g-dev
```

for RedHat based linux:

```
yum groupinstall 'Development Tools'
yum install git, automake, libtool, bison, flex readline-devel
```

on mac OS it enough to have XCode command line tools installed.

After that everything is ready to install postgres along with multimaster extension.

```
git clone https://github.com/postgrespro/postgres_cluster.git
cd postgres_cluster
./configure --prefix=/path/to/install && make -j 4 install
cd contrib/mmts && make install
```

```./configure``` here is standard postgres autotools script, so it possible to specify [any options](https://www.postgresql.org/docs/9.6/static/install-procedure.html) available in postgres. Also please ensure that /path/to/install/bin is enlisted in ```PATH``` environment variable for current user:

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

When things go more stable we will release prebuilt packages for major platforms.



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

1. To be able to run multimaster we need following changes to ```postgresql.conf```:

    ```
    ### General postgres option to let multimaster work

    wal_level = logical     # multimaster is build on top of
                            # logical replication and will not work otherwise
    max_connections = 100
    max_prepared_transactions = 300 # all transactions are implicitly two-phase, so that's
                                    # a good idea to set this equal to max_connections*N_nodes.
    max_wal_senders = 10       # at least the number of nodes
    max_replication_slots = 10 # at least the number of nodes
    max_worker_processes = 250 # Each node has:
                            #   N_nodes-1 receiver
                            #   N_nodes-1 sender
                            #   1 mtm-sender
                            #   1 mtm-receiver
                            # Also transactions executed at neighbour nodes can cause spawn of
                            # background pool worker at our node. At max this will be equal to
                            # sum of max_connections on neighbour nodes.

    ### Multimaster-specific options

    shared_preload_libraries = 'multimaster'
    multimaster.max_nodes = 3  # cluster size
    multimaster.node_id = 1    # the 1-based index of the node in the cluster
    multimaster.conn_strings = 'dbname=mydb user=myuser host=node1, dbname=mydb user=myuser host=node2, dbname=mydb user=myuser host=node3'
                               # comma-separated list of connection strings to neighbour nodes.
    ```

    Full description of all configuration parameters available in section [configuration](doc/configuration.md). Depending on network environment and expected usage patterns one can want to tweak parameters.

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

While multimaster is usable with default configuration optins several params may require tuning.

* Hearbeat timeouts — multimaster periodically send heartbeat packets to check availability of neighbour nodes. ```multimaster.heartbeat_send_timeout``` defines amount of time between sending heartbeats, while ```multimaster.heartbeat_recv_timeout``` sets amount of time following which node assumed to be disconnected if no hearbeats were received during this time. It's good idea to set ```multimaster.heartbeat_send_timeout``` based on typical ping latencies between you nodes. Small recv/senv ratio decraeases time of failure detection, but increases probability of false positive failure detection, so tupical packet loss ratio between nodes should be taken into account.

* Min/max recovery lag — when node is disconnected from the cluster other nodes will keep to collect WAL logs for disconnected node until size of WAL log will grow to ```multimaster.max_recovery_lag```. Upon reaching this threshold WAL logs for disconnected node will be deleted, automatic recovery will be no longer possible and disconnected node should be cloned manually from one of alive node by ```pg_basebackup```. Increasing ```multimaster.max_recovery_lag``` increases amount of time while automatic recovery is possible, but also increasing maximum disk usage during WAL collection. On the other hand ```multimaster.min_recovery_lag``` sets difference between acceptor and donor nodes before switching ordanary recovery to exclusive mode, when commits on donor node are stopped. This step is necessary to ensure that no new commits will happend during node promotion from recovery state to online state and nodes will be at sync after that.


## Monitoring

* `mtm.get_nodes_state()` -- show status of nodes on cluster
* `mtm.get_cluster_state()` -- show whole cluster status
* `mtm.get_cluster_info()` -- print some debug info

Read description of all management functions at [functions](doc/functions.md)

## Adding nodes to cluster
## Excluding nodes from cluster


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

Directory contrib/mmts also includes docker-compose.yml that is capable of building multi-master and starting 3 node cluster listening on port 15432, 15433 and 15434.

```
cd contrib/mmts
docker-compose up
```

### PgPro packages

When things go more stable we will release prebuilt packages for major platforms.



## Configuration

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
## Tuning configuration params
## Monitoring

* `mtm.get_nodes_state()` -- show status of nodes on cluster
* `mtm.get_cluster_state()` -- show whole cluster status
* `mtm.get_cluster_info()` -- print some debug info
* `mtm.make_table_local(relation regclass)` -- stop replication for a given table

Read description of all management functions at [functions](doc/functions.md)

## Adding nodes to cluster
## Excluding nodes from cluster


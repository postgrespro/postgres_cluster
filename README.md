# Postgres_cluster

[![Build Status](https://travis-ci.org/postgrespro/postgres_cluster.svg?branch=master)](https://travis-ci.org/postgrespro/postgres_cluster)

Various experiments with PostgreSQL clustering perfomed at PostgresPro.

This is mirror of postgres repo with several changes to the core and few extra extensions.

## Core changes:

* Transaction manager interface (eXtensible Transaction Manager, xtm). Generic interface to plug distributed transaction engines. More info at [[https://wiki.postgresql.org/wiki/DTM]] and [[http://www.postgresql.org/message-id/flat/F2766B97-555D-424F-B29F-E0CA0F6D1D74@postgrespro.ru]].
* Distributed deadlock detection API.
* Fast 2pc patch. More info at [[http://www.postgresql.org/message-id/flat/74355FCF-AADC-4E51-850B-47AF59E0B215@postgrespro.ru]]

## New extensions:

* pg_dtm. Transaction management by interaction with standalone coordinator (Arbiter or dtmd). [[https://wiki.postgresql.org/wiki/DTM#DTM_approach]]
* pg_tsdtm. Coordinator-less transaction management by tracking commit timestamps.
* multimaster. Synchronous multi-master replication based on logical_decoding and pg_dtm.


## Changed extension:

* postgres_fdw. Added support of pg_dtm.

## Deploying

For deploy and test postgres over a cluster we use ansible. In each extension directory one can find test subdirectory where we are storing tests and deploy scripts.


### Running tests on local cluster

To use it one need ansible hosts file with following groups:

farms/cluster.example:
```
[clients] # benchmark will start simultaneously on that nodes
server0.example.com
[nodes] # all that nodes will run postgres, dtmd/master will be deployed to first
server1.example.com
server2.example.com
server3.example.com
```

After you have proper hosts file you can deploy all stuff to servers:

```shell
# cd pg_dtm/tests
# ansible-playbook -i farms/sai deploy_layouts/cluster.yml
```

To perform dtmbench run:

```shell
# ansible-playbook -i farms/sai perf.yml -e nnodes=3 -e nconns=100
```

here nnodes is number of nudes that will be used for that test, nconns is the
number of connections to the backend.



## Running tests on Amazon ec2


In the case of amazon cloud there is no need in specific hosts file. Instead of it
we use script farms/ec2.py to get current instances running on you account. To use 
that script you need to specify you account key and access_key in ~/.boto.cfg (or in
any other place that described at http://boto.cloudhackers.com/en/latest/boto_config_tut.html)

To create VMs in cloud run:
```shell
# ansible-playbook -i farms/ec2.py deploy_layouts/ec2.yml
```
After that you should wait few minutes to have info about that instances in Amazon API. After 
that you can deploy postgres as usual:
```shell
# ansible-playbook -i farms/ec2.py deploy_layouts/cluster-ec2.yml
```
And to run a benchmark:
```shell
# ansible-playbook -i farms/sai perf-ec2.yml -e nnodes=3 -e nconns=100
```

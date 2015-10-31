# pg_dtm

### Design

This repo implements distributed transaction manager using Snapshot Sharing mechanism. General concepts and alternative approaches described in postgres wiki https://wiki.postgresql.org/wiki/DTM.

Backend-DTM protocol description can be found in [dtmd/README](dtmd/README).

### Installation

* Patch postgres using xtm.patch. After that build and install postgres in usual way.
```bash
cd ~/code/postgres
patch -p1 < ~/code/pg_dtm/xtm.patch
```
* Install pg_dtm extension.
```bash
export PATH=/path/to/pgsql/bin/:$PATH
cd ~/code/pg_dtm
make && make install
```
* Run dtmd.
```bash
cd ~/code/pg_dtm/dtmd
make
mkdir /tmp/clog
./bin/dtmd &
```
* To run something meaningful you need at leat two postgres instances. Also pg_dtm requires presense in ```shared_preload_libraries```.
```bash
initdb -D ./install/data1
initdb -D ./install/data2
echo "port = 5433" >> ./install/data2/postgresql.conf
echo "shared_preload_libraries = 'pg_dtm'" >> ./install/data1/postgresql.conf
echo "shared_preload_libraries = 'pg_dtm'" >> ./install/data2/postgresql.conf
pg_ctl -D ./install/data1 -l ./install/data1/log start
pg_ctl -D ./install/data2 -l ./install/data2/log start
```

#### Automatic provisioning

For a cluster-wide deploy we use ansible, more details in tests/deploy_layouts. (Ansible instructions will be later)

### Usage

Now cluster is running and you can use global tx between two nodes. Let's connect to postgres instances at different ports:

```sql
create extension pg_dtm; -- node1
create table accounts(user_id int, amount int); -- node1
insert into accounts (select 2*generate_series(1,100)-1, 0); -- node1, odd user_id's
    create extension pg_dtm; -- node2
    create table accounts(user_id int, amount int); -- node2
    insert into accounts (select 2*generate_series(1,100), 0); -- node2, even user_id's
select dtm_begin_transaction(); -- node1, returns global xid, e.g. 42
	select dtm_join_transaction(42); -- node2, join global tx
begin; -- node1
	begin; -- node2
update accounts set amount=amount-100 where user_id=1; -- node1, transfer money from user#1
	update accounts set amount=amount+100 where user_id=2; -- node2, to user#2
commit; -- node1, blocks until second commit happend
	commit; -- node2
```

### Consistency testing

To ensure consistency we use simple bank test: perform a lot of simultaneous transfers between accounts on different servers, while constantly checking total amount of money on all accounts. This test can be found in tests/perf.

```bash
> go run ./tests/perf/*
  -C value
    	Connection string (repeat for multiple connections)
  -a int
    	The number of bank accounts (default 100000)
  -b string
    	Backend to use. Possible optinos: transfers, fdw, pgshard, readers. (default "transfers")
  -g	Use DTM to keep global consistency
  -i	Init database
  -l	Use 'repeatable read' isolation level instead of 'read committed'
  -n int
    	The number updates each writer (reader in case of Reades backend) performs (default 10000)
  -p	Use parallel execs
  -r int
    	The number of readers (default 1)
  -s int
    	StartID. Script will update rows starting from this value
  -v	Show progress and other stuff for mortals
  -w int
    	The number of writers (default 8)
```

So previous installation can be initialized with:
```
go run ./tests/perf/*.go  \
-C "dbname=postgres port=5432" \
-C "dbname=postgres port=5433" \
-g -i
```
and tested with:
```
go run ./tests/perf/*.go  \
-C "dbname=postgres port=5432" \
-C "dbname=postgres port=5433" \
-g
```

### Using with postres_fdw.

We also provide a patch, that enables support of global transactions with postres_fdw. After patching and installing postres_fdw it is possible to run same test via fdw usig key ```-b fdw```.

### Using with pg_shard

Citus Data have branch in their pg_shard repo, that interacts with transaction manager. https://github.com/citusdata/pg_shard/tree/transaction_manager_integration
To use this feature one should have following line in postgresql.conf (or set it via GUC)
```
pg_shard.use_dtm_transactions = 1
```

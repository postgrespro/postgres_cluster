0) Design

General concept oberview. wiki link. This repo implements Snapshot Sharing mechnism.
Protocol description. README.protocol
Presentation

1) Installing

* patch postgres
* install extension
* configure two postgreses
* run dtmd
* run postgreses

1b) Automatic provisioning

* For a wide deploy we use ansible. Layouts/Farms. More details later.

2) Usage

now you can use global tx between this two nodes

table with two columns
```sql
example
```

3) Consistency testing

To ensure consistency we use simple bank test: perform a lot of simultaneous transfers between accounts on different servers, while constantly checking total amount of money on all accounts.

go run ...

also there is the test for measuring select performance.

4) Using with fdw.

patch
go run ...

5) Using with pg_shard

checkout repo
go run ...

6) Results

Some graphs

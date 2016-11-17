CREATE EXTENSION fasttrun;

create table persist ( a int );
insert into persist values (1);
select fasttruncate('persist');
insert into persist values (2);
select * from persist order by a;

create temp table temp1 (a int);
insert into temp1 values (1);

BEGIN;

create temp table temp2 (a int);
insert into temp2 values (1);

select * from temp1 order by a;
select * from temp2 order by a;

insert into temp1 (select * from generate_series(1,10000));
insert into temp2 (select * from generate_series(1,11000));

analyze temp2;
select relname,  relpages>0, reltuples>0 from pg_class where relname in ('temp1', 'temp2') order by relname;

select fasttruncate('temp1');
select fasttruncate('temp2');

insert into temp1 values (-2);
insert into temp2 values (-2);

select * from temp1 order by a;
select * from temp2 order by a;

COMMIT;

select * from temp1 order by a;
select * from temp2 order by a;

select relname,  relpages>0, reltuples>0 from pg_class where relname in ('temp1', 'temp2') order by relname;

select fasttruncate('temp1');
select fasttruncate('temp2');

select * from temp1 order by a;
select * from temp2 order by a;

select relname,  relpages>0, reltuples>0 from pg_class where relname in ('temp1', 'temp2') order by relname;

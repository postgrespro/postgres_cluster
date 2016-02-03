create table tmptab (pk integer primary key, sk integer);

-- insert ebnough records to make psotgresql optimizer use indexes
insert into tmptab values (generate_series(1, 10000), generate_series(1, 10000));

vacuum analyze;

-- create normal index
create index idx on tmptab(sk);

-- just normal index search
select * from tmptab where sk = 100;

-- make index partial
alter index idx where pk < 1000;

-- select using exact partial index range
select * from tmptab where sk = 100 and pk < 1000;
explain select * from tmptab where sk = 100 and pk < 1000;

-- select using subset of partial index range 
select * from tmptab where sk = 100 and pk < 200;
explain select * from tmptab where sk = 100 and pk < 200;

-- select outside partial index range 
select * from tmptab where sk = 100 and pk > 1000;
explain select * from tmptab where sk = 100 and pk > 1000;

-- select without partial index range
select * from tmptab where sk = 100;
explain select * from tmptab where sk = 100;

-- extend partial index range 
alter index idx where pk < 10000;

-- select using exact partial index range
select * from tmptab where sk = 1000 and pk < 10000;
explain select * from tmptab where sk = 1000 and pk < 10000;

-- calculating aggregate within exact partial index range
select count(*) from tmptab where sk < 1000 and pk < 10000;
explain select count(*) from tmptab where sk < 1000 and pk < 10000;

-- reducing partial idex predicate
alter index idx where pk < 9000;

-- select using new exact partial index range and key value belonging to old range
select * from tmptab where sk = 9000 and pk < 9000;
explain select * from tmptab where sk = 9000 and pk < 9000;

-- select using exact partial index range
select * from tmptab where sk = 900 and pk < 9000;
explain select * from tmptab where sk = 900 and pk < 9000;

drop table tmptab;

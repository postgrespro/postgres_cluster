select 'asd'::mchar::mvarchar;
select 'asd '::mchar::mvarchar;
select 'asd'::mchar(2)::mvarchar;
select 'asd '::mchar(2)::mvarchar;
select 'asd'::mchar(5)::mvarchar;
select 'asd '::mchar(5)::mvarchar;
select 'asd'::mchar::mvarchar(2);
select 'asd '::mchar::mvarchar(2);
select 'asd'::mchar(2)::mvarchar(2);
select 'asd '::mchar(2)::mvarchar(2);
select 'asd'::mchar(5)::mvarchar(2);
select 'asd '::mchar(5)::mvarchar(2);
select 'asd'::mchar::mvarchar(5);
select 'asd '::mchar::mvarchar(5);
select 'asd'::mchar(2)::mvarchar(5);
select 'asd '::mchar(2)::mvarchar(5);
select 'asd'::mchar(5)::mvarchar(5);
select 'asd '::mchar(5)::mvarchar(5);

select 'asd'::mvarchar::mchar;
select 'asd '::mvarchar::mchar;
select 'asd'::mvarchar(2)::mchar;
select 'asd '::mvarchar(2)::mchar;
select 'asd'::mvarchar(5)::mchar;
select 'asd '::mvarchar(5)::mchar;
select 'asd'::mvarchar::mchar(2);
select 'asd '::mvarchar::mchar(2);
select 'asd'::mvarchar(2)::mchar(2);
select 'asd '::mvarchar(2)::mchar(2);
select 'asd'::mvarchar(5)::mchar(2);
select 'asd '::mvarchar(5)::mchar(2);
select 'asd'::mvarchar::mchar(5);
select 'asd '::mvarchar::mchar(5);
select 'asd'::mvarchar(2)::mchar(5);
select 'asd '::mvarchar(2)::mchar(5);
select 'asd'::mvarchar(5)::mchar(5);
select 'asd '::mvarchar(5)::mchar(5);

select 'asd'::mchar || '123';
select 'asd'::mchar || '123'::mchar;
select 'asd'::mchar || '123'::mvarchar;

select 'asd '::mchar || '123';
select 'asd '::mchar || '123'::mchar;
select 'asd '::mchar || '123'::mvarchar;

select 'asd '::mchar || '123 ';
select 'asd '::mchar || '123 '::mchar;
select 'asd '::mchar || '123 '::mvarchar;


select 'asd'::mvarchar || '123';
select 'asd'::mvarchar || '123'::mchar;
select 'asd'::mvarchar || '123'::mvarchar;

select 'asd '::mvarchar || '123';
select 'asd '::mvarchar || '123'::mchar;
select 'asd '::mvarchar || '123'::mvarchar;

select 'asd '::mvarchar || '123 ';
select 'asd '::mvarchar || '123 '::mchar;
select 'asd '::mvarchar || '123 '::mvarchar;


select 'asd'::mchar(2) || '123';
select 'asd'::mchar(2) || '123'::mchar;
select 'asd'::mchar(2) || '123'::mvarchar;


select 'asd '::mchar(2) || '123';
select 'asd '::mchar(2) || '123'::mchar;
select 'asd '::mchar(2) || '123'::mvarchar;


select 'asd '::mchar(2) || '123 ';
select 'asd '::mchar(2) || '123 '::mchar;
select 'asd '::mchar(2) || '123 '::mvarchar;

select 'asd'::mvarchar(2) || '123';
select 'asd'::mvarchar(2) || '123'::mchar;
select 'asd'::mvarchar(2) || '123'::mvarchar;

select 'asd '::mvarchar(2) || '123';
select 'asd '::mvarchar(2) || '123'::mchar;
select 'asd '::mvarchar(2) || '123'::mvarchar;

select 'asd '::mvarchar(2) || '123 ';
select 'asd '::mvarchar(2) || '123 '::mchar;
select 'asd '::mvarchar(2) || '123 '::mvarchar;

select 'asd'::mchar(4) || '143';
select 'asd'::mchar(4) || '123'::mchar;
select 'asd'::mchar(4) || '123'::mvarchar;

select 'asd '::mchar(4) || '123';
select 'asd '::mchar(4) || '123'::mchar;
select 'asd '::mchar(4) || '123'::mvarchar;

select 'asd '::mchar(4) || '123 ';
select 'asd '::mchar(4) || '123 '::mchar;
select 'asd '::mchar(4) || '123 '::mvarchar;

select 'asd'::mvarchar(4) || '123';
select 'asd'::mvarchar(4) || '123'::mchar;
select 'asd'::mvarchar(4) || '123'::mvarchar;

select 'asd '::mvarchar(4) || '123';
select 'asd '::mvarchar(4) || '123'::mchar;
select 'asd '::mvarchar(4) || '123'::mvarchar;

select 'asd '::mvarchar(4) || '123 ';
select 'asd '::mvarchar(4) || '123 '::mchar;
select 'asd '::mvarchar(4) || '123 '::mvarchar;


select 'asd '::mvarchar(4) || '123 '::mchar(4);
select 'asd '::mvarchar(4) || '123 '::mvarchar(4);
select 'asd '::mvarchar(4) || '123'::mchar(4);
select 'asd '::mvarchar(4) || '123'::mvarchar(4);


select 1 where 'f'::mchar='F'::mvarchar;
select 1 where 'f'::mchar='F '::mvarchar;
select 1 where 'f '::mchar='F'::mvarchar;
select 1 where 'f '::mchar='F '::mvarchar;

select 1 where 'f'::mchar='F'::mvarchar(2);
select 1 where 'f'::mchar='F '::mvarchar(2);
select 1 where 'f '::mchar='F'::mvarchar(2);
select 1 where 'f '::mchar='F '::mvarchar(2);

select 1 where 'f'::mchar(2)='F'::mvarchar;
select 1 where 'f'::mchar(2)='F '::mvarchar;
select 1 where 'f '::mchar(2)='F'::mvarchar;
select 1 where 'f '::mchar(2)='F '::mvarchar;

select 1 where 'f'::mchar(2)='F'::mvarchar(2);
select 1 where 'f'::mchar(2)='F '::mvarchar(2);
select 1 where 'f '::mchar(2)='F'::mvarchar(2);
select 1 where 'f '::mchar(2)='F '::mvarchar(2);

select 1 where 'foo'::mchar='FOO'::mvarchar;
select 1 where 'foo'::mchar='FOO '::mvarchar;
select 1 where 'foo '::mchar='FOO'::mvarchar;
select 1 where 'foo '::mchar='FOO '::mvarchar;

select 1 where 'foo'::mchar='FOO'::mvarchar(2);
select 1 where 'foo'::mchar='FOO '::mvarchar(2);
select 1 where 'foo '::mchar='FOO'::mvarchar(2);
select 1 where 'foo '::mchar='FOO '::mvarchar(2);

select 1 where 'foo'::mchar(2)='FOO'::mvarchar;
select 1 where 'foo'::mchar(2)='FOO '::mvarchar;
select 1 where 'foo '::mchar(2)='FOO'::mvarchar;
select 1 where 'foo '::mchar(2)='FOO '::mvarchar;

select 1 where 'foo'::mchar(2)='FOO'::mvarchar(2);
select 1 where 'foo'::mchar(2)='FOO '::mvarchar(2);
select 1 where 'foo '::mchar(2)='FOO'::mvarchar(2);
select 1 where 'foo '::mchar(2)='FOO '::mvarchar(2);

Select 'f'::mchar(1) Union Select 'o'::mvarchar(1);
Select 'f'::mvarchar(1) Union Select 'o'::mchar(1);

select * from chvch where ch=vch;

select ch.* from ch, (select 'dEfg'::mvarchar as q) as p  where  chcol > p.q;
create index qq on ch (chcol);
set enable_seqscan=off;
select ch.* from ch, (select 'dEfg'::mvarchar as q) as p  where  chcol > p.q;
set enable_seqscan=on;


--\copy chvch to 'results/chvch.dump' binary
--truncate table chvch;
--\copy chvch from 'results/chvch.dump' binary

--test joins
CREATE TABLE a (mchar2 MCHAR(2) NOT NULL);
CREATE TABLE c (mvarchar255 mvarchar NOT NULL);
SELECT * FROM a, c WHERE mchar2 = mvarchar255;
SELECT * FROM a, c WHERE mvarchar255 = mchar2;
DROP TABLE a;
DROP TABLE c;


-- simplest examples
-- E061-04 like predicate
SELECT 'hawkeye'::mchar LIKE 'h%' AS "true";
SELECT 'hawkeye'::mchar NOT LIKE 'h%' AS "false";

SELECT 'hawkeye'::mchar LIKE 'H%' AS "true";
SELECT 'hawkeye'::mchar NOT LIKE 'H%' AS "false";

SELECT 'hawkeye'::mchar LIKE 'indio%' AS "false";
SELECT 'hawkeye'::mchar NOT LIKE 'indio%' AS "true";

SELECT 'hawkeye'::mchar LIKE 'h%eye' AS "true";
SELECT 'hawkeye'::mchar NOT LIKE 'h%eye' AS "false";

SELECT 'indio'::mchar LIKE '_ndio' AS "true";
SELECT 'indio'::mchar NOT LIKE '_ndio' AS "false";

SELECT 'indio'::mchar LIKE 'in__o' AS "true";
SELECT 'indio'::mchar NOT LIKE 'in__o' AS "false";

SELECT 'indio'::mchar LIKE 'in_o' AS "false";
SELECT 'indio'::mchar NOT LIKE 'in_o' AS "true";

SELECT 'hawkeye'::mvarchar LIKE 'h%' AS "true";
SELECT 'hawkeye'::mvarchar NOT LIKE 'h%' AS "false";

SELECT 'hawkeye'::mvarchar LIKE 'H%' AS "true";
SELECT 'hawkeye'::mvarchar NOT LIKE 'H%' AS "false";

SELECT 'hawkeye'::mvarchar LIKE 'indio%' AS "false";
SELECT 'hawkeye'::mvarchar NOT LIKE 'indio%' AS "true";

SELECT 'hawkeye'::mvarchar LIKE 'h%eye' AS "true";
SELECT 'hawkeye'::mvarchar NOT LIKE 'h%eye' AS "false";

SELECT 'indio'::mvarchar LIKE '_ndio' AS "true";
SELECT 'indio'::mvarchar NOT LIKE '_ndio' AS "false";

SELECT 'indio'::mvarchar LIKE 'in__o' AS "true";
SELECT 'indio'::mvarchar NOT LIKE 'in__o' AS "false";

SELECT 'indio'::mvarchar LIKE 'in_o' AS "false";
SELECT 'indio'::mvarchar NOT LIKE 'in_o' AS "true";

-- unused escape character
SELECT 'hawkeye'::mchar LIKE 'h%'::mchar ESCAPE '#' AS "true";
SELECT 'hawkeye'::mchar NOT LIKE 'h%'::mchar ESCAPE '#' AS "false";

SELECT 'indio'::mchar LIKE 'ind_o'::mchar ESCAPE '$' AS "true";
SELECT 'indio'::mchar NOT LIKE 'ind_o'::mchar ESCAPE '$' AS "false";

-- escape character
-- E061-05 like predicate with escape clause
SELECT 'h%'::mchar LIKE 'h#%'::mchar ESCAPE '#' AS "true";
SELECT 'h%'::mchar NOT LIKE 'h#%'::mchar ESCAPE '#' AS "false";

SELECT 'h%wkeye'::mchar LIKE 'h#%'::mchar ESCAPE '#' AS "false";
SELECT 'h%wkeye'::mchar NOT LIKE 'h#%'::mchar ESCAPE '#' AS "true";

SELECT 'h%wkeye'::mchar LIKE 'h#%%'::mchar ESCAPE '#' AS "true";
SELECT 'h%wkeye'::mchar NOT LIKE 'h#%%'::mchar ESCAPE '#' AS "false";

SELECT 'h%awkeye'::mchar LIKE 'h#%a%k%e'::mchar ESCAPE '#' AS "true";
SELECT 'h%awkeye'::mchar NOT LIKE 'h#%a%k%e'::mchar ESCAPE '#' AS "false";

SELECT 'indio'::mchar LIKE '_ndio'::mchar ESCAPE '$' AS "true";
SELECT 'indio'::mchar NOT LIKE '_ndio'::mchar ESCAPE '$' AS "false";

SELECT 'i_dio'::mchar LIKE 'i$_d_o'::mchar ESCAPE '$' AS "true";
SELECT 'i_dio'::mchar NOT LIKE 'i$_d_o'::mchar ESCAPE '$' AS "false";

SELECT 'i_dio'::mchar LIKE 'i$_nd_o'::mchar ESCAPE '$' AS "false";
SELECT 'i_dio'::mchar NOT LIKE 'i$_nd_o'::mchar ESCAPE '$' AS "true";

SELECT 'i_dio'::mchar LIKE 'i$_d%o'::mchar ESCAPE '$' AS "true";
SELECT 'i_dio'::mchar NOT LIKE 'i$_d%o'::mchar ESCAPE '$' AS "false";

-- escape character same as pattern character
SELECT 'maca'::mchar LIKE 'm%aca' ESCAPE '%'::mchar AS "true";
SELECT 'maca'::mchar NOT LIKE 'm%aca' ESCAPE '%'::mchar AS "false";

SELECT 'ma%a'::mchar LIKE 'm%a%%a' ESCAPE '%'::mchar AS "true";
SELECT 'ma%a'::mchar NOT LIKE 'm%a%%a' ESCAPE '%'::mchar AS "false";

SELECT 'bear'::mchar LIKE 'b_ear' ESCAPE '_'::mchar AS "true";
SELECT 'bear'::mchar NOT LIKE 'b_ear'::mchar ESCAPE '_' AS "false";

SELECT 'be_r'::mchar LIKE 'b_e__r' ESCAPE '_'::mchar AS "true";
SELECT 'be_r'::mchar NOT LIKE 'b_e__r' ESCAPE '_'::mchar AS "false";

SELECT 'be_r'::mchar LIKE '__e__r' ESCAPE '_'::mchar AS "false";
SELECT 'be_r'::mchar NOT LIKE '__e__r'::mchar ESCAPE '_' AS "true";

-- unused escape character
SELECT 'hawkeye'::mvarchar LIKE 'h%'::mvarchar ESCAPE '#' AS "true";
SELECT 'hawkeye'::mvarchar NOT LIKE 'h%'::mvarchar ESCAPE '#' AS "false";

SELECT 'indio'::mvarchar LIKE 'ind_o'::mvarchar ESCAPE '$' AS "true";
SELECT 'indio'::mvarchar NOT LIKE 'ind_o'::mvarchar ESCAPE '$' AS "false";

-- escape character
-- E061-05 like predicate with escape clause
SELECT 'h%'::mvarchar LIKE 'h#%'::mvarchar ESCAPE '#' AS "true";
SELECT 'h%'::mvarchar NOT LIKE 'h#%'::mvarchar ESCAPE '#' AS "false";

SELECT 'h%wkeye'::mvarchar LIKE 'h#%'::mvarchar ESCAPE '#' AS "false";
SELECT 'h%wkeye'::mvarchar NOT LIKE 'h#%'::mvarchar ESCAPE '#' AS "true";

SELECT 'h%wkeye'::mvarchar LIKE 'h#%%'::mvarchar ESCAPE '#' AS "true";
SELECT 'h%wkeye'::mvarchar NOT LIKE 'h#%%'::mvarchar ESCAPE '#' AS "false";

SELECT 'h%awkeye'::mvarchar LIKE 'h#%a%k%e'::mvarchar ESCAPE '#' AS "true";
SELECT 'h%awkeye'::mvarchar NOT LIKE 'h#%a%k%e'::mvarchar ESCAPE '#' AS "false";

SELECT 'indio'::mvarchar LIKE '_ndio'::mvarchar ESCAPE '$' AS "true";
SELECT 'indio'::mvarchar NOT LIKE '_ndio'::mvarchar ESCAPE '$' AS "false";

SELECT 'i_dio'::mvarchar LIKE 'i$_d_o'::mvarchar ESCAPE '$' AS "true";
SELECT 'i_dio'::mvarchar NOT LIKE 'i$_d_o'::mvarchar ESCAPE '$' AS "false";

SELECT 'i_dio'::mvarchar LIKE 'i$_nd_o'::mvarchar ESCAPE '$' AS "false";
SELECT 'i_dio'::mvarchar NOT LIKE 'i$_nd_o'::mvarchar ESCAPE '$' AS "true";

SELECT 'i_dio'::mvarchar LIKE 'i$_d%o'::mvarchar ESCAPE '$' AS "true";
SELECT 'i_dio'::mvarchar NOT LIKE 'i$_d%o'::mvarchar ESCAPE '$' AS "false";

-- escape character same as pattern character
SELECT 'maca'::mvarchar LIKE 'm%aca' ESCAPE '%'::mvarchar AS "true";
SELECT 'maca'::mvarchar NOT LIKE 'm%aca' ESCAPE '%'::mvarchar AS "false";

SELECT 'ma%a'::mvarchar LIKE 'm%a%%a' ESCAPE '%'::mvarchar AS "true";
SELECT 'ma%a'::mvarchar NOT LIKE 'm%a%%a' ESCAPE '%'::mvarchar AS "false";

SELECT 'bear'::mvarchar LIKE 'b_ear' ESCAPE '_'::mvarchar AS "true";
SELECT 'bear'::mvarchar NOT LIKE 'b_ear'::mvarchar ESCAPE '_' AS "false";

SELECT 'be_r'::mvarchar LIKE 'b_e__r' ESCAPE '_'::mvarchar AS "true";
SELECT 'be_r'::mvarchar NOT LIKE 'b_e__r' ESCAPE '_'::mvarchar AS "false";

SELECT 'be_r'::mvarchar LIKE '__e__r' ESCAPE '_'::mvarchar AS "false";
SELECT 'be_r'::mvarchar NOT LIKE '__e__r'::mvarchar ESCAPE '_' AS "true";

-- similar to

SELECT 'abc'::mchar SIMILAR TO 'abc'::mchar   AS   "true";
SELECT 'abc'::mchar SIMILAR TO 'a'::mchar      AS  "false";
SELECT 'abc'::mchar SIMILAR TO '%(b|d)%'::mchar AS "true";
SELECT 'abc'::mchar SIMILAR TO '(b|c)%'::mchar AS  "false";
SELECT 'h%'::mchar SIMILAR TO 'h#%'::mchar AS "false";
SELECT 'h%'::mchar SIMILAR TO 'h#%'::mchar ESCAPE '#' AS "true";

SELECT 'abc'::mvarchar SIMILAR TO 'abc'::mvarchar   AS   "true";
SELECT 'abc'::mvarchar SIMILAR TO 'a'::mvarchar      AS  "false";
SELECT 'abc'::mvarchar SIMILAR TO '%(b|d)%'::mvarchar AS "true";
SELECT 'abc'::mvarchar SIMILAR TO '(b|c)%'::mvarchar AS  "false";
SELECT 'h%'::mvarchar SIMILAR TO 'h#%'::mvarchar AS "false";
SELECT 'h%'::mvarchar SIMILAR TO 'h#%'::mvarchar ESCAPE '#' AS "true";

-- index support

SELECT * from ch where chcol like 'aB_d' order by chcol using &<;
SELECT * from ch where chcol like 'aB%d' order by chcol using &<;
SELECT * from ch where chcol like 'aB%' order by chcol using &<;
SELECT * from ch where chcol like '%BC%' order by chcol using &<;
set enable_seqscan = off;
SELECT * from ch where chcol like 'aB_d' order by chcol using &<;
SELECT * from ch where chcol like 'aB%d' order by chcol using &<;
SELECT * from ch where chcol like 'aB%' order by chcol using &<;
SELECT * from ch where chcol like '%BC%' order by chcol using &<;
set enable_seqscan = on;


create table testt (f1 mchar(10));
insert into testt values ('Abc-000001');
insert into testt values ('Abc-000002');
insert into testt values ('0000000001');
insert into testt values ('0000000002');

select f1 from testt where f1::mvarchar like E'Abc\\-%'::mvarchar;
select * from testt where f1::mchar like E'Abc\\-%'::mchar;
create index testindex on testt(f1);
set enable_seqscan=off;
select f1 from testt where f1::mvarchar like E'Abc\\-%'::mvarchar;
select * from testt where f1::mchar like E'Abc\\-%'::mchar;
set enable_seqscan = on;
drop table testt;

create table testt (f1 mvarchar(10));
insert into testt values ('Abc-000001');
insert into testt values ('Abc-000002');
insert into testt values ('0000000001');
insert into testt values ('0000000002');

select f1 from testt where f1::mvarchar like E'Abc\\-%'::mvarchar;
select * from testt where f1::mchar like E'Abc\\-%'::mchar;
select * from testt where f1::mchar like E'Abc\\-  %'::mchar;
select * from testt where f1::mchar like E'   %'::mchar;
create index testindex on testt(f1);
set enable_seqscan=off;
select f1 from testt where f1::mvarchar like E'Abc\\-%'::mvarchar;
select * from testt where f1::mchar like E'Abc\\-%'::mchar;
select * from testt where f1::mchar like E'Abc\\-   %'::mchar;
select * from testt where f1::mchar like E'   %'::mchar;
set enable_seqscan = on;
drop table testt;


CREATE TABLE test ( code mchar(5) NOT NULL );
insert into test values('1111 ');
insert into test values('111  ');
insert into test values('11   ');
insert into test values('1    ');

SELECT * FROM test WHERE code LIKE ('%    ');



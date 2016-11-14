
--
-- first, define the datatype.  Turn off echoing so that expected file
-- does not depend on contents of mchar.sql.
--

\set ECHO none
\i mchar.sql
--- load for table based checks
SET search_path = public;
\set ECHO all

create table ch (
	chcol mchar(32)
) without oids;

insert into ch values('abcd');
insert into ch values('AbcD');
insert into ch values('abcz');
insert into ch values('defg');
insert into ch values('dEfg');
insert into ch values('ee');
insert into ch values('Ee');

create table chvch (
    ch      mchar(12),
	vch     mvarchar(12)
) without oids;

insert into chvch values('No spaces', 'No spaces');
insert into chvch values('One space ', 'One space ');
insert into chvch values('1 space', '1 space ');


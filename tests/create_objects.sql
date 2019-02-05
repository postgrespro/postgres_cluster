CREATE SCHEMA tests;
SET search_path = 'tests';

CREATE TYPE int42;
-- Make dummy I/O routines using the existing internal support for int4, text
CREATE FUNCTION int42_in(cstring)
   RETURNS int42
   AS 'int4in'
   LANGUAGE internal STRICT IMMUTABLE;
CREATE FUNCTION int42_out(int42)
   RETURNS cstring
   AS 'int4out'
   LANGUAGE internal STRICT IMMUTABLE;

CREATE TYPE int42 (
   internallength = 4,
   input = int42_in,
   output = int42_out,
   alignment = int4,
   default = 42,
   passedbyvalue
);

-- RELOID, TYPEOID
CREATE TABLE tests.t1 (id int42);
CREATE TABLE t2 (id int, payload TEXT, par1 INT);

CREATE FUNCTION select1(tid INT) RETURNS VOID AS $$
BEGIN
    INSERT INTO tests.t2 (id, payload, par1) VALUES (1, 'qwe', 2);
END;
$$ LANGUAGE plpgsql;

-- COLLOID
CREATE COLLATION test1 (locale = 'en_US.utf8');
CREATE TABLE ttest1 (
	id serial,
    a text COLLATE test1,
    b text COLLATE test1
);
INSERT INTO ttest1 (a, b) VALUES ('one', 'one');
INSERT INTO ttest1 (a, b) VALUES ('one', 'two');

-- OPEROID
CREATE OPERATOR public.### (
   leftarg = numeric,
   rightarg = numeric,
   procedure = numeric_add
);

-- Different types and parameter types
CREATE TYPE bug_status AS ENUM ('new', 'open', 'closed');

CREATE TABLE public.bug (
    id serial,
    description TEXT,
    status bug_status
);

INSERT INTO public.bug (description, status) VALUES ('abc', 'open');
INSERT INTO public.bug (description, status) VALUES ('abc1', 'closed');

CREATE TABLE public.bug1 (
    id serial,
    status bug_status
);
INSERT INTO public.bug1 (status) VALUES ('new');
INSERT INTO public.bug1 (status) VALUES ('new');
INSERT INTO public.bug1 (status) VALUES ('closed');


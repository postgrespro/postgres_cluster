-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION raftable" to load this file. \quit

-- get
CREATE FUNCTION raftable(key varchar(64))
RETURNS text
AS 'MODULE_PATHNAME','raftable_sql_get'
LANGUAGE C;

-- set
CREATE FUNCTION raftable(key varchar(64), value text)
RETURNS void
AS 'MODULE_PATHNAME','raftable_sql_set'
LANGUAGE C;

-- list
CREATE FUNCTION raftable()
RETURNS table (key varchar(64), value text)
AS 'MODULE_PATHNAME','raftable_sql_list'
LANGUAGE C;

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION raftable" to load this file. \quit

-- get
CREATE FUNCTION raftable(key int)
RETURNS text
AS 'MODULE_PATHNAME','raftable_sql_get'
LANGUAGE C;

-- set
CREATE FUNCTION raftable(key int, value text)
RETURNS void
AS 'MODULE_PATHNAME','raftable_sql_set'
LANGUAGE C;

-- list
CREATE FUNCTION raftable()
RETURNS table (key int, value text)
AS 'MODULE_PATHNAME','raftable_sql_list'
LANGUAGE C;

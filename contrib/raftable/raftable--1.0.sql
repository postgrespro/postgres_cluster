-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION raftable" to load this file. \quit

-- get
CREATE FUNCTION raftable(key varchar(64), timeout_ms int)
RETURNS text
AS 'MODULE_PATHNAME','raftable_sql_get'
LANGUAGE C;

-- set
CREATE FUNCTION raftable(key varchar(64), value text, timeout_ms int)
RETURNS bool
AS 'MODULE_PATHNAME','raftable_sql_set'
LANGUAGE C;

-- configure the peer with the specified id
CREATE FUNCTION raftable_peer(id int, host text, port int)
RETURNS void
AS 'MODULE_PATHNAME','raftable_sql_peer'
LANGUAGE C;

-- start the worker as the peer with the specified id, returns pid of the started worker
CREATE FUNCTION raftable_start(id int)
RETURNS int
AS 'MODULE_PATHNAME','raftable_sql_start'
LANGUAGE C;

-- stop the worker
CREATE FUNCTION raftable_stop()
RETURNS void
AS 'MODULE_PATHNAME','raftable_sql_stop'
LANGUAGE C;

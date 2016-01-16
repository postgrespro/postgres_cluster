-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION multimaster" to load this file. \quit

CREATE FUNCTION mm_start_replication() RETURNS void
AS 'MODULE_PATHNAME','mm_start_replication'
LANGUAGE C;

CREATE FUNCTION mm_stop_replication() RETURNS void
AS 'MODULE_PATHNAME','mm_stop_replication'
LANGUAGE C;

CREATE FUNCTION mm_disable_node(node integer) RETURNS void
AS 'MODULE_PATHNAME','mm_disable_node'
LANGUAGE C;


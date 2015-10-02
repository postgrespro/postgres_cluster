-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_dtm" to load this file. \quit

CREATE FUNCTION dtm_begin_transaction() RETURNS integer
AS 'MODULE_PATHNAME','dtm_begin_transaction'
LANGUAGE C;

CREATE FUNCTION dtm_join_transaction(xid integer) RETURNS void
AS 'MODULE_PATHNAME','dtm_join_transaction'
LANGUAGE C;

CREATE FUNCTION dtm_get_current_snapshot_xmin() RETURNS integer
AS 'MODULE_PATHNAME','dtm_get_current_snapshot_xmin'
LANGUAGE C;

CREATE FUNCTION dtm_get_current_snapshot_xmax() RETURNS integer
AS 'MODULE_PATHNAME','dtm_get_current_snapshot_xmax'
LANGUAGE C;

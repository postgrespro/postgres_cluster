-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_dtm" to load this file. \quit

CREATE FUNCTION dtm_global_transaction(xids integer[]) RETURNS void
AS 'MODULE_PATHNAME','dtm_global_transaction'
LANGUAGE C;

CREATE FUNCTION dtm_get_snapshot() RETURNS void
AS 'MODULE_PATHNAME','dtm_get_snapshot'
LANGUAGE C;


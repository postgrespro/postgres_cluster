-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_dtm" to load this file. \quit

CREATE FUNCTION dtm_begin_transaction(nodes integer[], xids integer[]) RETURNS void
AS 'MODULE_PATHNAME','dtm_begin_transaction'
LANGUAGE C;

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_dtm" to load this file. \quit

CREATE FUNCTION dtm_register_node(id integer) RETURNS void
AS 'MODULE_PATHNAME','dtm_register_node'
LANGUAGE C;

CREATE FUNCTION dtm_extend(gtid cstring default null) RETURNS bigint
AS 'MODULE_PATHNAME','dtm_extend'
LANGUAGE C;

CREATE FUNCTION dtm_access(snapshot bigint, gtid cstring default null) RETURNS bigint
AS 'MODULE_PATHNAME','dtm_access'
LANGUAGE C;

CREATE FUNCTION dtm_begin_prepare(gtid cstring, coordinator integer) RETURNS void
AS 'MODULE_PATHNAME','dtm_begin_prepare'
LANGUAGE C;

CREATE FUNCTION dtm_prepare(gtid cstring, csn bigint) RETURNS bigint
AS 'MODULE_PATHNAME','dtm_prepare'
LANGUAGE C;

CREATE FUNCTION dtm_end_prepare(gtid cstring, csn bigint) RETURNS void
AS 'MODULE_PATHNAME','dtm_end_prepare'
LANGUAGE C;

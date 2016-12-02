-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use ”CREATE EXTENSION pg_transfer” to load this file. \quit

CREATE FUNCTION pg_transfer_wal(oid)
RETURNS void
AS 'MODULE_PATHNAME', 'pg_transfer_wal'
LANGUAGE C;

CREATE FUNCTION pg_transfer_cleanup_shmem(oid)
RETURNS void
AS 'MODULE_PATHNAME', 'pg_transfer_cleanup_shmem'
LANGUAGE C;

CREATE FUNCTION pg_transfer_freeze(oid, oid)
RETURNS void
AS 'MODULE_PATHNAME', 'pg_transfer_freeze'
LANGUAGE C;
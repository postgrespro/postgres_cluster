\echo Use "CREATE EXTENSION pg_execplan" to load this file. \quit

-- Store plan of a query into a text file.
-- query - query string which will be parsed and planned.
-- filename - path to the file on a disk.
CREATE OR REPLACE FUNCTION @extschema@.pg_store_query_plan(
														query TEXT,
														filename TEXT)
RETURNS VOID AS 'pg_execplan'
LANGUAGE C;

CREATE OR REPLACE FUNCTION @extschema@.pg_exec_query_plan(filename TEXT)
RETURNS VOID AS 'pg_execplan'
LANGUAGE C;

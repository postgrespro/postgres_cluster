\echo Use "CREATE EXTENSION pg_execplan" to load this file. \quit

CREATE OR REPLACE FUNCTION @extschema@.pg_store_query_plan(query TEXT)
RETURNS VOID AS 'pg_execplan'
LANGUAGE C;

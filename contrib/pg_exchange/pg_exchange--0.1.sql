\echo Use "CREATE EXTENSION pg_exchange" to load this file. \quit

CREATE OR REPLACE FUNCTION @extschema@.pg_exec_plan(query TEXT, plan TEXT, params TEXT, serverName TEXT)
RETURNS BOOL AS 'pg_exchange'
LANGUAGE C;

-- message queue receiver, for internal use only
CREATE FUNCTION dmq_receiver_loop(sender_name text) RETURNS void
AS 'MODULE_PATHNAME','dmq_receiver_loop'
LANGUAGE C;


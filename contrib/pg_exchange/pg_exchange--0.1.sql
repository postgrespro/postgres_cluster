\echo Use "CREATE EXTENSION pg_exchange" to load this file. \quit

CREATE OR REPLACE FUNCTION @extschema@.init_pg_exchange(id int)
RETURNS VOID AS 'pg_exchange'
LANGUAGE C;

-- message queue receiver, for internal use only
CREATE FUNCTION dmq_receiver_loop(sender_name text) RETURNS void
AS 'MODULE_PATHNAME','dmq_receiver_loop'
LANGUAGE C;

SELECT init_pg_exchange(1);

\echo Use "CREATE EXTENSION execplan" to load this file. \quit

--
-- Function ExecutePlan execute 'raw' query plan and ignore it's results.	
--
CREATE OR REPLACE FUNCTION @extschema@.pg_execute_plan(plan	TEXT)
RETURNS BOOL
AS 'MODULE_PATHNAME', 'pg_execute_plan'
LANGUAGE C STRICT;

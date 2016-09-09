/* contrib/sr_plan/sr_plan--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION sr_plan" to load this file. \quit

CREATE TABLE sr_plans (
	query_hash	int NOT NULL,
	plan_hash	int NOT NULL,
	query		varchar NOT NULL,
	plan		jsonb NOT NULL,
	enable		boolean NOT NULL,
	valid		boolean NOT NULL
);

CREATE INDEX sr_plans_query_hash_idx ON sr_plans (query_hash);
--CREATE INDEX sr_plans_plan_hash_idx ON sr_plans (plan_hashs);
--create function _p(anyelement) returns anyelement as $$ select $1; $$ language sql VOLATILE;

CREATE FUNCTION _p(anyelement)
RETURNS anyelement
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;


CREATE FUNCTION explain_jsonb_plan(jsonb)
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION sr_plan_invalid_table() RETURNS event_trigger
    AS 'MODULE_PATHNAME' LANGUAGE C;

CREATE EVENT TRIGGER sr_plan_invalid_table ON sql_drop
--	WHEN TAG IN ('DROP TABLE')
    EXECUTE PROCEDURE sr_plan_invalid_table();
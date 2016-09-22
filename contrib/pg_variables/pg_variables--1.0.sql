/* contrib/pg_variables/pg_variables--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_variables" to load this file. \quit

-- Scalar variables functions

CREATE FUNCTION pgv_set_int(package text, name text, value int)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_int'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_int(package text, name text, strict bool default true)
RETURNS int
AS 'MODULE_PATHNAME', 'variable_get_int'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_text(package text, name text, value text)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_text'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_text(package text, name text, strict bool default true)
RETURNS text
AS 'MODULE_PATHNAME', 'variable_get_text'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_numeric(package text, name text, value numeric)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_numeric'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_numeric(package text, name text, strict bool default true)
RETURNS numeric
AS 'MODULE_PATHNAME', 'variable_get_numeric'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_timestamp(package text, name text, value timestamp)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_timestamp'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_timestamp(package text, name text, strict bool default true)
RETURNS timestamp
AS 'MODULE_PATHNAME', 'variable_get_timestamp'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_timestamptz(package text, name text, value timestamptz)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_timestamptz'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_timestamptz(package text, name text, strict bool default true)
RETURNS timestamptz
AS 'MODULE_PATHNAME', 'variable_get_timestamptz'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_date(package text, name text, value date)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_date'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_date(package text, name text, strict bool default true)
RETURNS date
AS 'MODULE_PATHNAME', 'variable_get_date'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_set_jsonb(package text, name text, value jsonb)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_set_jsonb'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_get_jsonb(package text, name text, strict bool default true)
RETURNS jsonb
AS 'MODULE_PATHNAME', 'variable_get_jsonb'
LANGUAGE C VOLATILE;

-- Functions to work with records
CREATE FUNCTION pgv_insert(package text, name text, r record)
RETURNS void
AS 'MODULE_PATHNAME', 'variable_insert'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_update(package text, name text, r record)
RETURNS boolean
AS 'MODULE_PATHNAME', 'variable_update'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_delete(package text, name text, value anynonarray)
RETURNS boolean
AS 'MODULE_PATHNAME', 'variable_delete'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_select(package text, name text)
RETURNS setof record
AS 'MODULE_PATHNAME', 'variable_select'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_select(package text, name text, value anynonarray)
RETURNS record
AS 'MODULE_PATHNAME', 'variable_select_by_value'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_select(package text, name text, value anyarray)
RETURNS setof record
AS 'MODULE_PATHNAME', 'variable_select_by_values'
LANGUAGE C VOLATILE;

-- Functions to work with packages

CREATE FUNCTION pgv_exists(package text, name text)
RETURNS bool
AS 'MODULE_PATHNAME', 'variable_exists'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_remove(package text, name text)
RETURNS void
AS 'MODULE_PATHNAME', 'remove_variable'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_remove(package text)
RETURNS void
AS 'MODULE_PATHNAME', 'remove_package'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_free()
RETURNS void
AS 'MODULE_PATHNAME', 'remove_packages'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_list()
RETURNS TABLE(package text, name text)
AS 'MODULE_PATHNAME', 'get_packages_and_variables'
LANGUAGE C VOLATILE;

CREATE FUNCTION pgv_stats()
RETURNS TABLE(package text, allocated_memory bigint)
AS 'MODULE_PATHNAME', 'get_packages_stats'
LANGUAGE C VOLATILE;

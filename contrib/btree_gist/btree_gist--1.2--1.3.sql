/* contrib/btree_gist/btree_gist--1.2--1.3.sql */

-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION btree_gist UPDATE TO '1.3'" to load this file. \quit

-- update references to distance operators in pg_amop and pg_depend 

WITH
btree_ops AS (
	SELECT
		amoplefttype, amoprighttype, amopopr
	FROM 
		pg_amop
		JOIN pg_am ON pg_am.oid = amopmethod
		JOIN pg_opfamily ON pg_opfamily.oid = amopfamily
		JOIN pg_namespace ON pg_namespace.oid = opfnamespace
	WHERE
		nspname = 'pg_catalog'
		AND opfname IN (
			'integer_ops',
			'oid_ops',
			'money_ops',
			'float_ops',
			'datetime_ops',
			'time_ops',
			'interval_ops'
		)
		AND amname = 'btree'
		AND amoppurpose = 'o'
		AND amopstrategy = 6
	), 
gist_ops AS (
	SELECT
		pg_amop.oid AS oid, amoplefttype, amoprighttype, amopopr
	FROM
		pg_amop
		JOIN pg_am ON amopmethod = pg_am.oid
		JOIN pg_opfamily ON amopfamily = pg_opfamily.oid
		JOIN pg_namespace ON pg_namespace.oid = opfnamespace
	WHERE
		nspname = current_schema()
		AND opfname IN (
			'gist_oid_ops',
			'gist_int2_ops',
			'gist_int4_ops',
			'gist_int8_ops',
			'gist_float4_ops',
			'gist_float8_ops',
			'gist_timestamp_ops',
			'gist_timestamptz_ops',
			'gist_time_ops',
			'gist_date_ops',
			'gist_interval_ops',
			'gist_cash_ops'
		)
		AND amname = 'gist'
		AND amoppurpose = 'o'
		AND amopstrategy = 15
	),
depend_update_data(gist_amop, gist_amopopr, btree_amopopr) AS (
	SELECT 
		gist_ops.oid, gist_ops.amopopr, btree_ops.amopopr
	FROM 
		btree_ops JOIN gist_ops USING (amoplefttype, amoprighttype)
),
amop_update_data AS (
	UPDATE
		pg_depend
	SET
		refobjid = btree_amopopr
	FROM
		depend_update_data
	WHERE
		objid = gist_amop AND refobjid = gist_amopopr
	RETURNING
		depend_update_data.*
)
UPDATE
	pg_amop
SET
	amopopr = btree_amopopr
FROM
	amop_update_data
WHERE
	pg_amop.oid = gist_amop;

-- disable implicit pg_catalog search

DO
$$
BEGIN
	EXECUTE 'SET LOCAL search_path TO ' || current_schema() || ', pg_catalog';
END
$$;

-- drop distance operators

ALTER EXTENSION btree_gist DROP OPERATOR <-> (int2, int2);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (int4, int4);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (int8, int8);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (float4, float4);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (float8, float8);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (oid, oid);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (money, money);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (date, date);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (time, time);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (timestamp, timestamp);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (timestamptz, timestamptz);
ALTER EXTENSION btree_gist DROP OPERATOR <-> (interval, interval);

DROP OPERATOR <-> (int2, int2);
DROP OPERATOR <-> (int4, int4);
DROP OPERATOR <-> (int8, int8);
DROP OPERATOR <-> (float4, float4);
DROP OPERATOR <-> (float8, float8);
DROP OPERATOR <-> (oid, oid);
DROP OPERATOR <-> (money, money);
DROP OPERATOR <-> (date, date);
DROP OPERATOR <-> (time, time);
DROP OPERATOR <-> (timestamp, timestamp);
DROP OPERATOR <-> (timestamptz, timestamptz);
DROP OPERATOR <-> (interval, interval);

-- drop distance functions

ALTER EXTENSION btree_gist DROP FUNCTION int2_dist(int2, int2);
ALTER EXTENSION btree_gist DROP FUNCTION int4_dist(int4, int4);
ALTER EXTENSION btree_gist DROP FUNCTION int8_dist(int8, int8);
ALTER EXTENSION btree_gist DROP FUNCTION float4_dist(float4, float4);
ALTER EXTENSION btree_gist DROP FUNCTION float8_dist(float8, float8);
ALTER EXTENSION btree_gist DROP FUNCTION oid_dist(oid, oid);
ALTER EXTENSION btree_gist DROP FUNCTION cash_dist(money, money);
ALTER EXTENSION btree_gist DROP FUNCTION date_dist(date, date);
ALTER EXTENSION btree_gist DROP FUNCTION time_dist(time, time);
ALTER EXTENSION btree_gist DROP FUNCTION ts_dist(timestamp, timestamp);
ALTER EXTENSION btree_gist DROP FUNCTION tstz_dist(timestamptz, timestamptz);
ALTER EXTENSION btree_gist DROP FUNCTION interval_dist(interval, interval);

DROP FUNCTION int2_dist(int2, int2);
DROP FUNCTION int4_dist(int4, int4);
DROP FUNCTION int8_dist(int8, int8);
DROP FUNCTION float4_dist(float4, float4);
DROP FUNCTION float8_dist(float8, float8);
DROP FUNCTION oid_dist(oid, oid);
DROP FUNCTION cash_dist(money, money);
DROP FUNCTION date_dist(date, date);
DROP FUNCTION time_dist(time, time);
DROP FUNCTION ts_dist(timestamp, timestamp);
DROP FUNCTION tstz_dist(timestamptz, timestamptz);
DROP FUNCTION interval_dist(interval, interval);

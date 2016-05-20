CREATE EXTENSION pg_variables;

-- Integer variables
SELECT pgv_get_int('vars', 'int1');
SELECT pgv_get_int('vars', 'int1', false);

SELECT pgv_set_int('vars', 'int1', 101);
SELECT pgv_set_int('vars', 'int2', 102);

SELECT pgv_get_int('vars', 'int1');
SELECT pgv_get_int('vars', 'int2');
SELECT pgv_set_int('vars', 'int1', 103);
SELECT pgv_get_int('vars', 'int1');

SELECT pgv_get_int('vars', 'int3');
SELECT pgv_get_int('vars', 'int3', false);
SELECT pgv_exists('vars', 'int3');
SELECT pgv_exists('vars', 'int1');

SELECT pgv_set_int('vars', 'intNULL', NULL);
SELECT pgv_get_int('vars', 'intNULL');

-- Text variables
SELECT pgv_set_text('vars', 'str1', 's101');
SELECT pgv_set_text('vars', 'int1', 's101');
SELECT pgv_set_int('vars', 'str1', 101);
SELECT pgv_set_text('vars', 'str2', 's102');

SELECT pgv_get_text('vars', 'str1');
SELECT pgv_get_text('vars', 'str2');
SELECT pgv_set_text('vars', 'str1', 's103');
SELECT pgv_get_text('vars', 'str1');

SELECT pgv_get_text('vars', 'str3');
SELECT pgv_get_text('vars', 'str3', false);
SELECT pgv_exists('vars', 'str3');
SELECT pgv_exists('vars', 'str1');
SELECT pgv_get_text('vars', 'int1');
SELECT pgv_get_int('vars', 'str1');

SELECT pgv_set_text('vars', 'strNULL', NULL);
SELECT pgv_get_text('vars', 'strNULL');

-- Numeric variables
SELECT pgv_set_numeric('vars', 'num1', 1.01);
SELECT pgv_set_numeric('vars', 'num2', 1.02);
SELECT pgv_set_numeric('vars', 'str1', 1.01);

SELECT pgv_get_numeric('vars', 'num1');
SELECT pgv_get_numeric('vars', 'num2');
SELECT pgv_set_numeric('vars', 'num1', 1.03);
SELECT pgv_get_numeric('vars', 'num1');

SELECT pgv_get_numeric('vars', 'num3');
SELECT pgv_get_numeric('vars', 'num3', false);
SELECT pgv_exists('vars', 'num3');
SELECT pgv_exists('vars', 'num1');
SELECT pgv_get_numeric('vars', 'str1');

SELECT pgv_set_numeric('vars', 'numNULL', NULL);
SELECT pgv_get_numeric('vars', 'numNULL');

SET timezone = 'Europe/Moscow';

-- Timestamp variables
SELECT pgv_set_timestamp('vars', 'ts1', '2016-03-30 10:00:00');
SELECT pgv_set_timestamp('vars', 'ts2', '2016-03-30 11:00:00');
SELECT pgv_set_timestamp('vars', 'num1', '2016-03-30 12:00:00');

SELECT pgv_get_timestamp('vars', 'ts1');
SELECT pgv_get_timestamp('vars', 'ts2');
SELECT pgv_set_timestamp('vars', 'ts1', '2016-03-30 12:00:00');
SELECT pgv_get_timestamp('vars', 'ts1');

SELECT pgv_get_timestamp('vars', 'ts3');
SELECT pgv_get_timestamp('vars', 'ts3', false);
SELECT pgv_exists('vars', 'ts3');
SELECT pgv_exists('vars', 'ts1');
SELECT pgv_get_timestamp('vars', 'num1');

SELECT pgv_set_timestamp('vars', 'tsNULL', NULL);
SELECT pgv_get_timestamp('vars', 'tsNULL');

-- TimestampTZ variables

SELECT pgv_set_timestamptz('vars', 'tstz1', '2016-03-30 10:00:00 GMT+01');
SELECT pgv_set_timestamptz('vars', 'tstz2', '2016-03-30 11:00:00 GMT+02');
SELECT pgv_set_timestamptz('vars', 'ts1', '2016-03-30 12:00:00 GMT+03');

SELECT pgv_get_timestamptz('vars', 'tstz1');
SELECT pgv_get_timestamptz('vars', 'tstz2');
SELECT pgv_set_timestamptz('vars', 'tstz1', '2016-03-30 12:00:00 GMT+01');
SELECT pgv_get_timestamptz('vars', 'tstz1');

SELECT pgv_get_timestamptz('vars', 'tstz3');
SELECT pgv_get_timestamptz('vars', 'tstz3', false);
SELECT pgv_exists('vars', 'tstz3');
SELECT pgv_exists('vars', 'tstz1');
SELECT pgv_get_timestamptz('vars', 'ts1');

SELECT pgv_set_timestamptz('vars', 'tstzNULL', NULL);
SELECT pgv_get_timestamptz('vars', 'tstzNULL');

-- Date variables
SELECT pgv_set_date('vars', 'd1', '2016-03-29');
SELECT pgv_set_date('vars', 'd2', '2016-03-30');
SELECT pgv_set_date('vars', 'tstz1', '2016-04-01');

SELECT pgv_get_date('vars', 'd1');
SELECT pgv_get_date('vars', 'd2');
SELECT pgv_set_date('vars', 'd1', '2016-04-02');
SELECT pgv_get_date('vars', 'd1');

SELECT pgv_get_date('vars', 'd3');
SELECT pgv_get_date('vars', 'd3', false);
SELECT pgv_exists('vars', 'd3');
SELECT pgv_exists('vars', 'd1');
SELECT pgv_get_date('vars', 'tstz1');

SELECT pgv_set_date('vars', 'dNULL', NULL);
SELECT pgv_get_date('vars', 'dNULL');

-- Jsonb variables
SELECT pgv_set_jsonb('vars2', 'j1', '[1, 2, "foo", null]');
SELECT pgv_set_jsonb('vars2', 'j2', '{"bar": "baz", "balance": 7.77, "active": false}');
SELECT pgv_set_jsonb('vars', 'd1', '[1, 2, "foo", null]');

SELECT pgv_get_jsonb('vars2', 'j1');
SELECT pgv_get_jsonb('vars2', 'j2');
SELECT pgv_set_jsonb('vars2', 'j1', '{"foo": [true, "bar"], "tags": {"a": 1, "b": null}}');
SELECT pgv_get_jsonb('vars2', 'j1');

SELECT pgv_get_jsonb('vars2', 'j3');
SELECT pgv_get_jsonb('vars2', 'j3', false);
SELECT pgv_exists('vars2', 'j3');
SELECT pgv_exists('vars2', 'j1');
SELECT pgv_get_jsonb('vars', 'd1');

SELECT pgv_set_jsonb('vars', 'jNULL', NULL);
SELECT pgv_get_jsonb('vars', 'jNULL');

-- Record variables
CREATE TABLE tab (id int, t varchar);
INSERT INTO tab VALUES (0, 'str00'), (1, 'str11'), (2, NULL), (NULL, 'strNULL');

SELECT pgv_insert('vars3', 'r1', tab) FROM tab;
SELECT pgv_insert('vars2', 'j1', tab) FROM tab;
SELECT pgv_insert('vars3', 'r1', tab) FROM tab;

SELECT pgv_insert('vars3', 'r1', row(1, 'str1', 'str2'));
SELECT pgv_insert('vars3', 'r1', row(1, 1));
SELECT pgv_insert('vars3', 'r1', row('str1', 'str1'));

SELECT pgv_select('vars3', 'r1');
SELECT pgv_select('vars3', 'r1', 1);
SELECT pgv_select('vars3', 'r1', 0);
SELECT pgv_select('vars3', 'r1', NULL::int);
SELECT pgv_select('vars3', 'r1', ARRAY[1, 0, NULL]);

UPDATE tab SET t = 'str33' WHERE id = 1;
SELECT pgv_update('vars3', 'r1', tab) FROM tab;
SELECT pgv_select('vars3', 'r1');

SELECT pgv_delete('vars3', 'r1', 1);
SELECT pgv_select('vars3', 'r1');
SELECT pgv_delete('vars3', 'r1', 100);

SELECT pgv_select('vars3', 'r3');
SELECT pgv_exists('vars3', 'r3');
SELECT pgv_exists('vars3', 'r1');
SELECT pgv_select('vars2', 'j1');

-- Manipulate variables
SELECT * FROM pgv_list() order by package, name;

SELECT pgv_remove('vars', 'int3');
SELECT pgv_remove('vars', 'int1');
SELECT pgv_get_int('vars', 'int1');

SELECT pgv_remove('vars2');
SELECT pgv_get_jsonb('vars2', 'j1');

SELECT * FROM pgv_list() order by package, name;

SELECT pgv_free();

SELECT * FROM pgv_list() order by package, name;

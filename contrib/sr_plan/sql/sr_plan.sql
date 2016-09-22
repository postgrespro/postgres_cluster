CREATE EXTENSION sr_plan;

CREATE TABLE test_table(test_attr1 int, test_attr2 int);
SET sr_plan.write_mode = true;
SELECT * FROM test_table WHERE test_attr1 = _p(10);
SELECT * FROM test_table WHERE test_attr1 = 10;
SELECT * FROM test_table WHERE test_attr1 = 10;
SET sr_plan.write_mode = false;

SELECT * FROM test_table WHERE test_attr1 = _p(10);
SELECT * FROM test_table WHERE test_attr1 = 10;
SELECT * FROM test_table WHERE test_attr1 = 15;

UPDATE sr_plans SET enable = true;

SELECT * FROM test_table WHERE test_attr1 = _p(10);
SELECT * FROM test_table WHERE test_attr1 = _p(15);
SELECT * FROM test_table WHERE test_attr1 = 10;
SELECT * FROM test_table WHERE test_attr1 = 15;

DROP TABLE test_table;
CREATE TABLE test_table(test_attr1 int, test_attr2 int);

SELECT * FROM test_table WHERE test_attr1 = _p(10);
SELECT * FROM test_table WHERE test_attr1 = 10;
SELECT * FROM test_table WHERE test_attr1 = 10;
SELECT * FROM test_table WHERE test_attr1 = 15;
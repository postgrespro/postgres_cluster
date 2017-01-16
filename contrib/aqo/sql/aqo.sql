CREATE TABLE aqo_test0(a int, b int, c int, d int);
INSERT INTO aqo_test0 VALUES (0, 0, 0, 0);
INSERT INTO aqo_test0 VALUES (1, 1, 1, 1);
INSERT INTO aqo_test0 VALUES (3, 3, 3, 3);
INSERT INTO aqo_test0 VALUES (5, 5, 5, 5);
INSERT INTO aqo_test0 VALUES (6, 6, 6, 6);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
INSERT INTO aqo_test0 (SELECT * FROM aqo_test0);
ANALYZE aqo_test0;

CREATE EXTENSION aqo;

SET aqo.mode = 'manual';

EXPLAIN 
SELECT * FROM aqo_test0
WHERE a < 3 AND b < 3 AND c < 3 AND d < 3;

EXPLAIN
SELECT * FROM aqo_test0
WHERE a < 5 AND b < 5 AND c < 5 AND d < 5;

SET aqo.mode = 'forced';

CREATE TABLE tmp1 AS SELECT * FROM aqo_test0
WHERE a < 3 AND b < 3 AND c < 3 AND d < 3;
SELECT count(*) FROM tmp1;
DROP TABLE tmp1;

CREATE TABLE tmp1 AS SELECT * FROM aqo_test0
WHERE a < 5 AND b < 5 AND c < 5 AND d < 5;
SELECT count(*) FROM tmp1;
DROP TABLE tmp1;

EXPLAIN
SELECT * FROM aqo_test0
WHERE a < 3 AND b < 3 AND c < 3 AND d < 3;

EXPLAIN
SELECT * FROM aqo_test0
WHERE a < 5 AND b < 5 AND c < 5 AND d < 5;

DROP TABLE aqo_test0;

DROP EXTENSION aqo;

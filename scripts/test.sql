INSERT INTO t1 (b) VALUES (3);
INSERT INTO t1 (b) VALUES (15);
INSERT INTO t1 (id, b) VALUES (3, 15);

-- Check params serialization
--PREPARE fooplan (numeric, numeric) AS INSERT INTO t1 VALUES($1, $2);
--EXECUTE fooplan(13, 200);


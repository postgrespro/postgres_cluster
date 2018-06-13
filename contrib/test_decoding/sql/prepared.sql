-- predictability
SET synchronous_commit = on;
SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot', 'test_decoding');

CREATE TABLE test_prepared1(id integer primary key);
CREATE TABLE test_prepared2(id integer primary key);

-- test simple successful use of a prepared xact
BEGIN;
INSERT INTO test_prepared1 VALUES (1);
PREPARE TRANSACTION 'test_prepared#1';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');
COMMIT PREPARED 'test_prepared#1';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');
INSERT INTO test_prepared1 VALUES (2);

-- test abort of a prepared xact
BEGIN;
INSERT INTO test_prepared1 VALUES (3);
PREPARE TRANSACTION 'test_prepared#2';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');
ROLLBACK PREPARED 'test_prepared#2';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

INSERT INTO test_prepared1 VALUES (4);

-- test prepared xact containing ddl
BEGIN;
INSERT INTO test_prepared1 VALUES (5);
ALTER TABLE test_prepared1 ADD COLUMN data text;
INSERT INTO test_prepared1 VALUES (6, 'frakbar');
PREPARE TRANSACTION 'test_prepared#3';

SELECT 'test_prepared_1' AS relation, locktype, mode
FROM pg_locks
WHERE locktype = 'relation'
  AND relation = 'test_prepared1'::regclass;

SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- Test that we decode correctly while an uncommitted prepared xact
-- with ddl exists.
--
-- Use a separate table for the concurrent transaction because the lock from
-- the ALTER will stop us inserting into the other one.
--
-- We should see '7' before '5' in our results since it commits first.
--
INSERT INTO test_prepared2 VALUES (7);
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

COMMIT PREPARED 'test_prepared#3';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- make sure stuff still works
INSERT INTO test_prepared1 VALUES (8);
INSERT INTO test_prepared2 VALUES (9);
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- Check `CLUSTER` (as operation that hold exclusive lock) doesn't block
-- logical decoding.
BEGIN;
INSERT INTO test_prepared1 VALUES (10, 'othercol');
CLUSTER test_prepared1 USING test_prepared1_pkey;
INSERT INTO test_prepared1 VALUES (11, 'othercol2');
PREPARE TRANSACTION 'test_prepared_lock';

BEGIN;
insert into test_prepared2 values (12);
PREPARE TRANSACTION 'test_prepared_lock2';
COMMIT PREPARED 'test_prepared_lock2';

SELECT 'pg_class' AS relation, locktype, mode
FROM pg_locks
WHERE locktype = 'relation'
  AND relation = 'pg_class'::regclass;

-- Shouldn't timeout on 2pc decoding.
SET statement_timeout = '1s';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');
RESET statement_timeout;

COMMIT PREPARED 'test_prepared_lock';

-- will work normally after we commit
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- test savepoints 
BEGIN;
SAVEPOINT test_savepoint;
CREATE TABLE test_prepared_savepoint (a int);
PREPARE TRANSACTION 'test_prepared_savepoint';
COMMIT PREPARED 'test_prepared_savepoint';
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- test that a GID containing "nodecode" gets decoded at commit prepared time
BEGIN;
INSERT INTO test_prepared1 VALUES (20);
PREPARE TRANSACTION 'test_prepared_nodecode';
-- should show nothing
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');
COMMIT PREPARED 'test_prepared_nodecode';
-- should be decoded now
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

-- cleanup
DROP TABLE test_prepared1;
DROP TABLE test_prepared2;

-- show results. There should be nothing to show
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

SELECT pg_drop_replication_slot('regression_slot');

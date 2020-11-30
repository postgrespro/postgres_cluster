-- predictability
SET synchronous_commit = on;
SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot', 'test_decoding');
SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot_2pc', 'test_decoding');
SELECT 'init' FROM pg_create_logical_replication_slot('regression_slot_2pc_nofilter', 'test_decoding');

CREATE TABLE test_prepared1(id integer primary key);
CREATE TABLE test_prepared2(id integer primary key);

-- Reused queries
\set get_no2pc 'SELECT data FROM pg_logical_slot_get_changes(''regression_slot'', NULL, NULL, ''include-xids'', ''0'', ''skip-empty-xacts'', ''1'');'
\set get_with2pc 'SELECT data FROM pg_logical_slot_get_changes(''regression_slot_2pc'', NULL, NULL, ''include-xids'', ''0'', ''skip-empty-xacts'', ''1'', ''twophase-decoding'', ''1'');'
\set get_with2pc_nofilter 'SELECT data FROM pg_logical_slot_get_changes(''regression_slot_2pc_nofilter'', NULL, NULL, ''include-xids'', ''0'', ''skip-empty-xacts'', ''1'', ''twophase-decoding'', ''1'', ''twophase-decode-with-catalog-changes'', ''1'');'

-- test simple successful use of a prepared xact
BEGIN;
INSERT INTO test_prepared1 VALUES (1);
PREPARE TRANSACTION 'test_prepared#1';
:get_no2pc
:get_with2pc
:get_with2pc_nofilter
COMMIT PREPARED 'test_prepared#1';
:get_no2pc
:get_with2pc
:get_with2pc_nofilter
INSERT INTO test_prepared1 VALUES (2);

-- test abort of a prepared xact
BEGIN;
INSERT INTO test_prepared1 VALUES (3);
PREPARE TRANSACTION 'test_prepared#2';
:get_no2pc
:get_with2pc
:get_with2pc_nofilter
ROLLBACK PREPARED 'test_prepared#2';
:get_no2pc
:get_with2pc
:get_with2pc_nofilter

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

:get_no2pc
:get_with2pc
:get_with2pc_nofilter

-- Test that we decode correctly while an uncommitted prepared xact
-- with ddl exists. Our 2pc filter callback will skip decoding of xacts
-- with catalog changes at PREPARE time, so we don't decode it now.
--
-- Use a separate table for the concurrent transaction because the lock from
-- the ALTER will stop us inserting into the other one.
--
-- We should see '7' before '5' in our results since it commits first.
--
INSERT INTO test_prepared2 VALUES (7);
:get_no2pc
:get_with2pc
:get_with2pc_nofilter

COMMIT PREPARED 'test_prepared#3';
:get_no2pc
:get_with2pc
:get_with2pc_nofilter

-- make sure stuff still works
INSERT INTO test_prepared1 VALUES (8);
INSERT INTO test_prepared2 VALUES (9);
:get_no2pc
:get_with2pc
:get_with2pc_nofilter

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

-- Shouldn't see anything with 2pc decoding off
:get_no2pc

-- Shouldn't timeout on 2pc decoding.
SET statement_timeout = '1s';
:get_with2pc
:get_with2pc_nofilter
RESET statement_timeout;

COMMIT PREPARED 'test_prepared_lock';

-- Both will work normally after we commit
:get_no2pc
:get_with2pc
:get_with2pc_nofilter

-- cleanup
DROP TABLE test_prepared1;
DROP TABLE test_prepared2;

-- show results
SELECT data FROM pg_logical_slot_get_changes('regression_slot', NULL, NULL, 'include-xids', '0', 'skip-empty-xacts', '1');

SELECT pg_drop_replication_slot('regression_slot');
SELECT pg_drop_replication_slot('regression_slot_2pc');
SELECT pg_drop_replication_slot('regression_slot_2pc_nofilter');

/* First test whether a table's replication set can be properly manipulated */
SELECT * FROM pglogical_regress_variables()
\gset

\c :provider_dsn
SELECT pglogical.replicate_ddl_command($$
CREATE TABLE public.test2pc_tbl(id serial primary key, value int);
$$);
SELECT * FROM pglogical.replication_set_add_all_tables('default', '{public}');


-- Check that prapeared state is visible on slave and data available after commit
BEGIN;
INSERT INTO test2pc_tbl VALUES (1, 10);
PREPARE TRANSACTION 'tx1';
SELECT pg_xlog_wait_remote_apply(pg_current_xlog_location(), 0);

\c :subscriber_dsn
SELECT gid, owner, database FROM pg_prepared_xacts;
SELECT * FROM test2pc_tbl;

\c :provider_dsn
COMMIT PREPARED 'tx1';
SELECT pg_xlog_wait_remote_apply(pg_current_xlog_location(), 0);

\c :subscriber_dsn
SELECT gid, owner, database FROM pg_prepared_xacts;
SELECT * FROM test2pc_tbl;


-- Check that prapeared state is visible on slave and data is ignored after abort
\c :provider_dsn
BEGIN;
INSERT INTO test2pc_tbl VALUES (2, 20);
PREPARE TRANSACTION 'tx2';
SELECT pg_xlog_wait_remote_apply(pg_current_xlog_location(), 0);

\c :subscriber_dsn
SELECT gid, owner, database FROM pg_prepared_xacts;
SELECT * FROM test2pc_tbl;

\c :provider_dsn
ROLLBACK PREPARED 'tx2';
SELECT pg_xlog_wait_remote_apply(pg_current_xlog_location(), 0);

\c :subscriber_dsn
SELECT gid, owner, database FROM pg_prepared_xacts;
SELECT * FROM test2pc_tbl;

-- Clean up
\set VERBOSITY terse
SELECT pglogical.replicate_ddl_command($$
	DROP TABLE public.test2pc_tbl CASCADE;
$$);

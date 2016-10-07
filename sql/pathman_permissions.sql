\set VERBOSITY terse

CREATE EXTENSION pg_pathman;
CREATE ROLE user1 LOGIN;
CREATE ROLE user2 LOGIN;

SET ROLE user1;

CREATE TABLE user1_table(id serial, a int);
INSERT INTO user1_table SELECT g, g FROM generate_series(1, 20) as g;

ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO public;

SET ROLE user2;

/* Not owner and not superuser cannot create partitions */
SELECT create_range_partitions('user1_table', 'id', 1, 10, 2);

SET ROLE user1;

/* Owner can */
SELECT create_range_partitions('user1_table', 'id', 1, 10, 2);

SET ROLE user2;

/* Try to change partitioning parameters for user1_table */
SELECT set_enable_parent('user1_table', true);
SELECT set_auto('user1_table', false);

/*
 * Check that user is able to propagate partitions by inserting rows that
 * doesn't fit into range
 */
INSERT INTO user1_table (id, a) VALUES (25, 0);

SET ROLE user1;

SELECT drop_partitions('test1_table');

SET ROLE user2;

/* Test ddl event trigger */
CREATE TABLE user2_table(id serial);
SELECT create_hash_partitions('user2_table', 'id', 3);
INSERT INTO user2_table SELECT generate_series(1, 30);
SELECT drop_partitions('user2_table');

RESET ROLE;

DROP OWNED BY user1;
DROP OWNED BY user2;
DROP USER user1;
DROP USER user2;

\set VERBOSITY terse
SET search_path = 'public';

/* Try to create table without pg_pathman extension installed */
CREATE TABLE abc(id SERIAL)
PARTITION BY RANGE(id) INTERVAL (100) (PARTITION abc_1 VALUES LESS THAN (100));

CREATE EXTENSION pg_pathman;

/* Create interval partitioned table */
CREATE TABLE abc(id SERIAL)
PARTITION BY RANGE(id) INTERVAL (100)
(
    PARTITION abc_inf VALUES LESS THAN (100),
    PARTITION abc_100 VALUES LESS THAN (200)
);

ALTER TABLE abc ADD PARTITION abc_200 VALUES LESS THAN (400);
ALTER TABLE abc SPLIT PARTITION abc_200 AT (300) INTO (PARTITION abc_200, PARTITION abc_300);
ALTER TABLE abc MERGE PARTITIONS abc_100, abc_200, abc_300 INTO PARTITION abc_100;
SELECT * FROM pathman_partition_list;

/* Drop partition should shift next partition's lower bound */
ALTER TABLE abc ADD PARTITION abc_400 VALUES LESS THAN (500);
ALTER TABLE abc DROP PARTITION abc_100;
SELECT * FROM pathman_partition_list;



/* Inserting values into area not covered by partitions should create new partition */
INSERT INTO abc VALUES (550);
SELECT * FROM pathman_partition_list;

DROP TABLE abc CASCADE;

/*
 * Create range partitioned table (in contrast to interval-partitioned in terms
 * of Oracle)
 */
CREATE TABLE abc(id SERIAL)
PARTITION BY RANGE(id)
(
    PARTITION abc_inf VALUES LESS THAN (100),
    PARTITION abc_100 VALUES LESS THAN (200)
);

/* Inserting should produce an error because interval hasn't been set */
INSERT INTO abc VALUES (250);
DROP TABLE abc CASCADE;

/* Create hash partitioned table */
CREATE TABLE abc (id serial)
PARTITION BY HASH (id) PARTITIONS (4);

SELECT * FROM pathman_partition_list;

DROP TABLE abc CASCADE;

/* Create hash partitioned table */
CREATE TABLE abc(id serial)
PARTITION BY HASH (id) PARTITIONS (3);

SELECT * FROM pathman_partition_list;

DROP TABLE abc CASCADE;

CREATE TABLE abc(id serial);
INSERT INTO abc SELECT generate_series(1, 1000);

ALTER TABLE abc PARTITION BY RANGE (id) START FROM (1) INTERVAL (100);
SELECT * FROM pathman_partition_list;
SELECT drop_partitions('abc');

ALTER TABLE abc PARTITION BY HASH (id) PARTITIONS (3);
SELECT * FROM pathman_partition_list;

DROP TABLE abc CASCADE;

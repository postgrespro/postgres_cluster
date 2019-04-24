CREATE SERVER remote1 FOREIGN DATA WRAPPER postgres_fdw	OPTIONS (port '5432', use_remote_estimate 'on');
CREATE USER MAPPING FOR PUBLIC SERVER remote1;
CREATE SERVER remote2 FOREIGN DATA WRAPPER postgres_fdw OPTIONS (port '5433', use_remote_estimate 'on');
CREATE USER MAPPING FOR PUBLIC SERVER remote2;

DROP TABLE IF EXISTS pt cascade;
CREATE TABLE pt (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY hash (id);

CREATE FOREIGN TABLE pt_0 PARTITION OF pt FOR VALUES WITH (modulus 3, remainder 0) SERVER remote1;
CREATE FOREIGN TABLE pt_1 PARTITION OF pt FOR VALUES WITH (modulus 3, remainder 1) SERVER remote2;
CREATE TABLE pt_2 PARTITION OF pt FOR VALUES WITH (modulus 3, remainder 2);

DROP TABLE IF EXISTS rt cascade;
CREATE TABLE rt (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY hash (id);

CREATE FOREIGN TABLE rt_0 PARTITION OF rt FOR VALUES WITH (modulus 3, remainder 0) SERVER remote1;
CREATE FOREIGN TABLE rt_1 PARTITION OF rt FOR VALUES WITH (modulus 3, remainder 1) SERVER remote2;
CREATE TABLE rt_2 PARTITION OF rt FOR VALUES WITH (modulus 3, remainder 2);

DROP TABLE IF EXISTS st cascade;
CREATE TABLE st (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY hash (id);

CREATE FOREIGN TABLE st_0 PARTITION OF st FOR VALUES WITH (modulus 3, remainder 0) SERVER remote1;
CREATE FOREIGN TABLE st_1 PARTITION OF st FOR VALUES WITH (modulus 3, remainder 1) SERVER remote2;
CREATE TABLE st_2 PARTITION OF st FOR VALUES WITH (modulus 3, remainder 2);

-- On scan types.
DROP TABLE IF EXISTS t1 cascade;
CREATE TABLE t1 (
    id SERIAL,
    payload INTEGER
) PARTITION BY hash (id);

CREATE FOREIGN TABLE t1_0 PARTITION OF t1 FOR VALUES WITH (modulus 3, remainder 0) SERVER remote1;
CREATE FOREIGN TABLE t1_1 PARTITION OF t1 FOR VALUES WITH (modulus 3, remainder 1) SERVER remote2;
CREATE TABLE t1_2 PARTITION OF t1 FOR VALUES WITH (modulus 3, remainder 2);

CREATE INDEX ON t1_2(id);

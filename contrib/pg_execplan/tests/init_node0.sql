DROP TABLE IF EXISTS pt cascade;
CREATE TABLE pt (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY HASH(id);
CREATE TABLE pt_0 PARTITION OF pt FOR VALUES WITH (modulus 2, remainder 0);
CREATE FOREIGN TABLE pt_1 PARTITION OF pt FOR VALUES WITH (modulus 2, remainder 1) SERVER fdwremote;

DROP TABLE IF EXISTS rt cascade;
CREATE TABLE rt (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY HASH(id);
CREATE TABLE rt_0 PARTITION OF rt FOR VALUES WITH (modulus 2, remainder 0);
CREATE FOREIGN TABLE rt_1 PARTITION OF rt FOR VALUES WITH (modulus 2, remainder 1) SERVER fdwremote;

CREATE TABLE a (
    a1 integer NOT NULL,
    a2 integer
) PARTITION BY HASH(a1);
CREATE TABLE a0 PARTITION OF a FOR VALUES WITH (modulus 2, remainder 0);
CREATE TABLE a1 PARTITION OF a FOR VALUES WITH (modulus 2, remainder 1);

CREATE TABLE b (
    b1 integer NOT NULL,
    b2 integer
) PARTITION BY HASH(b1);
CREATE TABLE b0 PARTITION OF b FOR VALUES WITH (modulus 2, remainder 0);
CREATE TABLE b1 PARTITION OF b FOR VALUES WITH (modulus 2, remainder 1);

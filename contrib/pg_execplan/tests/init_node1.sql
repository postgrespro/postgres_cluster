DROP TABLE IF EXISTS pt cascade;
CREATE TABLE pt (
    id integer not null,
    payload integer,
    test integer
) PARTITION BY hash (id);

CREATE FOREIGN TABLE pt_0 PARTITION OF pt FOR VALUES WITH (modulus 2, remainder 0) SERVER fdwremote;
CREATE TABLE pt_1 PARTITION OF pt FOR VALUES WITH (modulus 2, remainder 1);

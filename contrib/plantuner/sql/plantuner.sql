LOAD 'plantuner';

SHOW	plantuner.disable_index;

CREATE TABLE wow (i int, j int);
CREATE INDEX i_idx ON wow (i);
CREATE INDEX j_idx ON wow (j);

SET enable_seqscan=off;

SELECT * FROM wow;

SET plantuner.disable_index="i_idx, j_idx";

SELECT * FROM wow;

SHOW plantuner.disable_index;

SET plantuner.disable_index="i_idx, nonexistent, public.j_idx, wow";

SHOW plantuner.disable_index;

SET plantuner.enable_index="i_idx";

SHOW plantuner.enable_index;

SELECT * FROM wow;

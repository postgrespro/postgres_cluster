CREATE EXTENSION hunspell_nl_nl;

CREATE TABLE table1(name varchar);
INSERT INTO table1 VALUES ('deuren'), ('deurtje'), ('deur'),
						('twee'), ('tweehonderd'), ('tweeduizend');

SELECT d.* FROM table1 AS t, LATERAL ts_debug('dutch_hunspell', t.name) AS d;

CREATE INDEX name_idx ON table1 USING GIN (to_tsvector('dutch_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('dutch_hunspell', name)
	@@ to_tsquery('dutch_hunspell', 'deurtje');
SELECT * FROM table1 WHERE to_tsvector('dutch_hunspell', name)
	@@ to_tsquery('dutch_hunspell', 'twee');

DROP INDEX name_idx;
CREATE INDEX name_idx ON table1 USING GIST (to_tsvector('dutch_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('dutch_hunspell', name)
	@@ to_tsquery('dutch_hunspell', 'deurtje');
SELECT * FROM table1 WHERE to_tsvector('dutch_hunspell', name)
	@@ to_tsquery('dutch_hunspell', 'twee');

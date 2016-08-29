CREATE EXTENSION hunspell_ru_ru;

CREATE TABLE table1(name varchar);
INSERT INTO table1 VALUES ('земля'), ('землей'), ('землями'), ('земли'),
						('туши'), ('тушь'), ('туша'), ('тушат'),
						('тушью');

SELECT d.* FROM table1 AS t, LATERAL ts_debug('russian_hunspell', t.name) AS d;

CREATE INDEX name_idx ON table1 USING GIN (to_tsvector('russian_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'землей');
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'тушь');
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'туша');

DROP INDEX name_idx;
CREATE INDEX name_idx ON table1 USING GIST (to_tsvector('russian_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'землей');
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'тушь');
SELECT * FROM table1 WHERE to_tsvector('russian_hunspell', name)
	@@ to_tsquery('russian_hunspell', 'туша');

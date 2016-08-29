CREATE EXTENSION hunspell_fr;

CREATE TABLE table1(name varchar);
INSERT INTO table1 VALUES ('batifoler'), ('batifolant'), ('batifole'), ('batifolait'),
						('consentant'), ('consentir'), ('consentiriez');

SELECT d.* FROM table1 AS t, LATERAL ts_debug('french_hunspell', t.name) AS d;

CREATE INDEX name_idx ON table1 USING GIN (to_tsvector('french_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('french_hunspell', name)
	@@ to_tsquery('french_hunspell', 'batifolant');
SELECT * FROM table1 WHERE to_tsvector('french_hunspell', name)
	@@ to_tsquery('french_hunspell', 'consentiriez');

DROP INDEX name_idx;
CREATE INDEX name_idx ON table1 USING GIST (to_tsvector('french_hunspell', "name"));
SELECT * FROM table1 WHERE to_tsvector('french_hunspell', name)
	@@ to_tsquery('french_hunspell', 'batifolant');
SELECT * FROM table1 WHERE to_tsvector('french_hunspell', name)
	@@ to_tsquery('french_hunspell', 'consentiriez');

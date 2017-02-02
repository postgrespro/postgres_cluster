# pg_tsparser - parser for text search

## Introduction

The **pg_tsparser** module is the modified default text search parser from
PostgreSQL 9.6. The differences are:
* **tsparser** gives unbroken words by underscore character
* **tsparser** gives unbroken words with numbers and letters by hyphen character

For example:

```sql
SELECT to_tsvector('english', 'pg_trgm') as def_parser,
       to_tsvector('english_ts', 'pg_trgm')  as new_parser;
   def_parser    |         new_parser
-----------------+-----------------------------
 'pg':1 'trgm':2 | 'pg':2 'pg_trgm':1 'trgm':3
(1 row)

SELECT to_tsvector('english', '123-abc') as def_parser,
       to_tsvector('english_ts', '123-abc')  as new_parser;
   def_parser    |         new_parser
-----------------+-----------------------------
 '123':1 'abc':2 | '123':2 '123-abc':1 'abc':3
(1 row)
```

## License

This module available under the same license as
[PostgreSQL](http://www.postgresql.org/about/licence/).

## Installation

Typical installation procedure may look like this:

    $ cd pg_tsparser
    $ sudo make USE_PGXS=1 install
    $ make USE_PGXS=1 installcheck
    $ psql DB -c "CREATE EXTENSION pg_tsparser;"

After this you can create your own text search configuration:

```sql
CREATE TEXT SEARCH CONFIGURATION english_ts (
    PARSER = tsparser
);

COMMENT ON TEXT SEARCH CONFIGURATION english_ts IS 'text search configuration for english language';

ALTER TEXT SEARCH CONFIGURATION english_ts
    ADD MAPPING FOR email, file, float, host, hword_numpart, int,
    numhword, numword, sfloat, uint, url, url_path, version
    WITH simple;

ALTER TEXT SEARCH CONFIGURATION english_ts
    ADD MAPPING FOR asciiword, asciihword, hword_asciipart,
    word, hword, hword_part
    WITH english_stem;
```

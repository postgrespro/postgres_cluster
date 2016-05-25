# pg_variables - session variables with various types

## Introduction

The **pg_variables** module provides functions to work with variables of various
types. Created variables live only in the current user session.

## License

This module available under the same license as
[PostgreSQL](http://www.postgresql.org/about/licence/).

## Installation

Typical installation procedure may look like this:

    $ cd pg_variables
    $ make USE_PGXS=1
    $ sudo make USE_PGXS=1 install
    $ make USE_PGXS=1 installcheck
    $ psql DB -c "CREATE EXTENSION pg_variables;"

## Module functions

### Integer variables

Function | Returns
-------- | -------
`pgv_set_int(package text, name text, value int)` | `void`
`pgv_get_int(package text, name text, strict bool default true)` | `int`

### Text variables

Function | Returns
-------- | -------
`pgv_set_text(package text, name text, value text)` | `void`
`pgv_get_text(package text, name text, strict bool default true)` | `text`

### Numeric variables

Function | Returns
-------- | -------
`pgv_set_numeric(package text, name text, value numeric)` | `void`
`pgv_get_numeric(package text, name text, strict bool default true)` | `numeric`

### Timestamp variables

Function | Returns
-------- | -------
`pgv_set_timestamp(package text, name text, value timestamp)` | `void`
`pgv_get_timestamp(package text, name text, strict bool default true)` | `timestamp`

### Timestamp with timezone variables

Function | Returns
-------- | -------
`pgv_set_timestamptz(package text, name text, value timestamptz)` | `void`
`pgv_get_timestamptz(package text, name text, strict bool default true)` | `timestamptz`

### Date variables

Function | Returns
-------- | -------
`pgv_set_date(package text, name text, value date)` | `void`
`pgv_get_date(package text, name text, strict bool default true)` | `date`

### Jsonb variables

Function | Returns
-------- | -------
`pgv_set_jsonb(package text, name text, value jsonb)` | `void`
`pgv_get_jsonb(package text, name text, strict bool default true)` | `jsonb`

### Records

Function | Returns
-------- | -------
`pgv_insert(package text, name text, r record)` | `void`
`pgv_update(package text, name text, r record)` | `boolean`
`pgv_delete(package text, name text, value anynonarray)` | `boolean`
`pgv_select(package text, name text)` | `set of record`
`pgv_select(package text, name text, value anynonarray)` | `record`
`pgv_select(package text, name text, value anyarray)` | `set of record`

### Miscellaneous functions

Function | Returns
-------- | -------
`pgv_exists(package text, name text)` | `bool`
`pgv_remove(package text, name text)` | `void`
`pgv_remove(package text)` | `void`
`pgv_free()` | `void`
`pgv_list()` | `table(package text, name text)`
`pgv_stats()` | `table(package text, used_memory bigint)`

Note that **pgv_stats()** works only with the PostgreSQL 9.6 and newer.

## Examples

It is easy to use functions to work with scalar variables:

```sql
SELECT pgv_set_int('vars', 'int1', 101);
SELECT pgv_set_int('vars', 'int2', 102);

SELECT pgv_get_int('vars', 'int1');
 pgv_get_int 
-------------
         101
(1 row)

SELECT pgv_get_int('vars', 'int2');
 pgv_get_int 
-------------
         102
(1 row)
```

Let's assume we have a **tab** table:

```sql
CREATE TABLE tab (id int, t varchar);
INSERT INTO tab VALUES (0, 'str00'), (1, 'str11');
```

Then you can use functions to work with record variables:

```sql
SELECT pgv_insert('vars', 'r1', tab) FROM tab;

SELECT pgv_select('vars', 'r1');
 pgv_select
------------
 (1,str11)
 (0,str00)
(2 rows)

SELECT pgv_select('vars', 'r1', 1);
 pgv_select
------------
 (1,str11)
(1 row)

SELECT pgv_select('vars', 'r1', 0);
 pgv_select
------------
 (0,str00)
(1 row)

SELECT pgv_select('vars', 'r1', ARRAY[1, 0]);
 pgv_select
------------
 (1,str11)
 (0,str00)
(2 rows)

SELECT pgv_delete('vars', 'r1', 1);

SELECT pgv_select('vars', 'r1');
 pgv_select
------------
 (0,str00)
(1 row)
```

You can list packages and variables:

```sql
SELECT * FROM pgv_list() order by package, name;
 package | name 
---------+------
 vars    | int1
 vars    | int2
 vars    | r1
(3 rows)
```

And get used memory in bytes:

```sql
SELECT * FROM pgv_stats() order by package;
 package | used_memory
---------+-------------
 vars    |       16736
(1 row)
```

You can delete variables or hole packages:

```sql
SELECT pgv_remove('vars', 'int1');
SELECT pgv_remove('vars');
```

You can delete all packages and variables:
```sql
SELECT pgv_free();
```

# Save and restore query plans in PostgreSQL

## Rationale

sr_plan looks like Oracle Outline system. It can be used to lock 
the execution plan. It is necessary if you do not trust the planner 
or able to form a better plan.

Typically, DBA would play with queries interactively, and save their
plans and then enable use of saved plans for the queries, where
predictable responce time is essential.

Then application which uses these queries would use saved plans.

## Build

This module needs to serialize and deserialize lot of structures, which
results in almost same code.

So, mako preprocessor is used to generate C files serialize.c and
deserialize.c.

If you want to modify these files, you'll needed to alter mako templates
and regenerate files, so you'll need to install mako, python >= 3.2 and
pycparser modules.

If you only have a Python you can use the virtual environment:
```bash
virtualenv env
source ./env/bin/activate
pip install -r ./requirements.txt
```

Then you need to generate C code and compile it:

```bash
make USE_PGXS=1 genparser
make USE_PGXS=1
make USE_PGXS=1 install
```

If you want to only build this module as is, pregenerated files are
provided for you, so ``make genparser`` command should be omitted.

## Installation

In your db:
```SQL
CREATE EXTENSION sr_plan;
```
and modify your postgresql.conf:
```
shared_preload_libraries = 'sr_plan.so'
```
It is essential that library is preloaded during server startup, because
use of saved plans is enabled on per-database basis and doesn't require
any per-connection actions.

## Usage

If you want to save the query plan is necessary to set the variable:

```SQL
set sr_plan.write_mode = true;
```

Now plans for all subsequent queries will be stored in the table sr_plans,
until this variable is set to false. Don't forget that all queries will be 
stored including duplicates.
Making an example query:

```SQL
select query_hash from sr_plans where query_hash=10;
```

Disable saving the query:

```SQL
set sr_plan.write_mode = false;
```
Now verify that your query is saved:
```SQL
select query_hash, enable, valid, query, explain_jsonb_plan(plan) from sr_plans;

 query_hash | enable | valid |                        query                         |                 explain_jsonb_plan                 
------------+--------+-------+------------------------------------------------------+----------------------------------------------------
 1783086253 | f      | t     | select query_hash from sr_plans where query_hash=10; | Bitmap Heap Scan on sr_plans                      +
            |        |       |                                                      |   Recheck Cond: (query_hash = 10)                 +
            |        |       |                                                      |   ->  Bitmap Index Scan on sr_plans_query_hash_idx+
            |        |       |                                                      |         Index Cond: (query_hash = 10)             +
            |        |       |                                                      | 

```

Note use of explain\_jsonb\_plan function, that  allows you to visualize
execution plan in the similar way as EXPLAIN command does.

In the database plans are stored as jsonb. By default, all the newly
saved plans are disabled, you need enable it manually:

To enable use of the saved plan 

```SQL
update sr_plans set enable=true where query_hash=1783086253;
```

(1783086253 for example only)
After that, the plan for the query will be taken from the sr_plans.

In addition sr plan allows you to save a parameterized query plan. In
this case, we have some constants in the query that, as we know, do
not affect plan.

During plan saving mode we can mark these constants as query parameters
using a special function _p (anyelement). For example:


```SQL

=>create table test_table (a numeric, b text);
CREATE TABLE
=>insert into test_table values (1,'1'),(2,'2'),(3,'3');
INSERT 0 3 
=> set sr_plan.write_mode = true;
SET
=> select a,b from test_table where a = _p(1);
 a | b
---+---
 1 | 1
(1 row)

=> set sr_plan.write_mode = false;
SET

```

Now plan for query from our table is saved with parameter. So,
if we enable saved plan in this table, this plan would be used for query
with any value for a, as long as this value is wrapped with _p()
function.

```SQL
=>update sr_plans set enable = true where quesry=
  'select a,b from test_table where a = _p(1)';
UPDATE 1
-- These queries would use saved plan

=>select a,b from test_table where a = _p(2);
 a | b
---+---
 2 | 2
(1 row)

=>select a,b from test_table where a = _p(3);
 a | b
---+---
 3 | 3
(1 row)

-- This query wouldn't use saved plan, because constant is not wrapped
-- with _p()

=>select a,b from test_table where a = 1;
 a | b
---+---
 1 | 1
(1 row)

```

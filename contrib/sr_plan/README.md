# Save and restore query plans in PostgreSQL

## Rationale

sr_plan looks like Oracle Outline system. It can be used to lock the execution plan. It is necessary if you do not trust the planner or able to form a better plan.

## Build

Dependencies: >= Python 3.2, Mako, pycparser 
If you only have a Python you can use the virtual environment:
```bash
virtualenv env
source ./env/bin/activate
pip install -r ./requirements.txt
```

Then you need to generate C code and compiled it:
```bash
make USE_PGXS=1 genparser
make USE_PGXS=1
make USE_PGXS=1 install
```

and modify your postgres config:
```
shared_preload_libraries = 'sr_plan.so'
```

## Usage
In your db:
```SQL
CREATE EXTENSION sr_plan;
```
If you want to save the query plan is necessary to set the variable:
```SQL
set sr_plan.write_mode = true;
```
Now plans for all subsequent queries will be stored in the table sr_plans. Don't forget that all queries will be stored including duplicates.
Making an example query:
```SQL
select query_hash from sr_plans where query_hash=10;
```
disable saving the query:
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

explain_jsonb_plan function allows you to display explain execute the plan of which lies in jsonb. By default, all the plans are off, you need enable it:
```SQL
update sr_plans set enable=true where query_hash=1783086253;
```
(1783086253 for example only)
After that, the plan for the query will be taken from the sr_plans.

In addition sr plan allows you to save a parameterized query plan. In this case, we have some constants in the query are not essential.
For the parameters we use a special function _p (anyelement) example:
```SQL
select query_hash from sr_plans where query_hash=1000+_p(10);
```
if we keep the plan for the query and enable it to be used also for the following queries:
```SQL
select query_hash from sr_plans where query_hash=1000+_p(11);
select query_hash from sr_plans where query_hash=1000+_p(-5);
```

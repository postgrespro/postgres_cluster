# Save and restore query plans in PostgreSQL

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
Now plans for all subsequent requests will be stored in the table sr_plans. It must be remembered that all requests will be maintained including duplicates.
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
select query_hash, enable, query, explain_jsonb_plan(plan) from sr_plans;
```
explain_jsonb_plan function allows you to display explain execute the plan of which lies in jsonb. By default, all the plans are off, you need enable it:
```SQL
update sr_plans set enable=true where query_hash=812619660;
```
(812619660 for example only)
After that, the plan for the query will be taken from the sr_plans.

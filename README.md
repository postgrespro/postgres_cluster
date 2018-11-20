# execplan
PostgreSQL Extension for raw query plan execution

This project dedicated to parallel query execution problem.

Parallel DBMS needs to execute an identical query plan at each computing node. It is needed for tuples redistribution during a query.
Today PostgreSQL can process only SQL strings. But it is not guaranteed, that the planner at each node will construct same query plan, because different statistics, relation sizes e.t.c.

# pg_repeater
PostgreSQL patch & extension for UTILITY queries and query plans execution at
remote instance.

Plan is passed by postgres_fdw connection service. It executed by pg_exec_plan()
routine, introduced by pg_execplan extension.

This project dedicated to query execution problem in DBMS for computing systems
with cluster architecture.

The DBMS may need to execute an identical query plan at each computing node.
Today PostgreSQL can process only SQL statements. But it is not guaranteed, that
the planner at each node will construct same query plan, because different
statistics, relation sizes e.t.c.

This solution based on postgres-xl approach: plan tree is serialized by the
nodeToString() routine.
During serialization we transform all database object identifiers (oid) at each
node field to portable representation.


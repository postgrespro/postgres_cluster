# execplan
PostgreSQL patch & extension for raw query plan execution

This project dedicated to query execution problem in DBMS for computing systems with cluster architecture.

The DBMS may need to execute an identical query plan at each computing node.
Today PostgreSQL can process only SQL statements. But it is not guaranteed, that the planner at each node will construct same query plan, because different statistics, relation sizes e.t.c.

This solution based on postgres-xl approach: plan tree is serialized by the nodeToString() routine.
During serialization we transform all database object identifiers (oid) in each node field to portable representation.
Further, the serialized plan transfer by new libpq routine called `PQsendPlan`.
In this project we use postgres_fdw connections for management of sessions and remote transactions.
Some `repeater` extension used for the demonstration of plan transfer machinery.
The `pg12_devel.patch` patch contains all core changes.
The `scripts` directory contains some simplistic demo tests.

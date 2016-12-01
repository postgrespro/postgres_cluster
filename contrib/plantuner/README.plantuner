Plantuner - enable planner hints

   contrib/plantuner is a contribution module for PostgreSQL 8.4+, which
   enable planner hints.

   All work was done by Teodor Sigaev (teodor@sigaev.ru) and Oleg Bartunov
   (oleg@sai.msu.su).

   Sponsor: Nomao project (http://www.nomao.com)

Motivation

   Whether somebody think it's bad or not, but sometime it's very
   interesting to be able to control planner (provide hints, which tells
   optimizer to ignore its algorithm in part), which is currently
   impossible in POstgreSQL. Oracle, for example, has over 120 hints, SQL
   Server also provides hints.

   This first version of plantuner provides a possibility to hide
   specified indexes from PostgreSQL planner, so it will not use them.

   There are many situation, when developer want to temporarily disable
   specific index(es), without dropping them, or to instruct planner to
   use specific index.

   Next, for some workload PostgreSQL could be too pessimistic for
   newly created tables and assumes much more rows in table than
   it actually has. If plantuner.fix_empty_table GUC variable is set
   to true then module will set to zero number of pages/tuples of
   table which hasn't blocks in file.

Installation

     * Get latest source of plantuner from CVS Repository
     * gmake && gmake install && gmake installcheck

Syntax
	plantuner.forbid_index (deprecated)
	plantuner.disable_index
		List of indexes invisible to planner
	plantuner.enable_index
		List of indexes visible to planner even they are hided
		by plantuner.disable_index. 

Usage

   To enable the module you can either load shared library 'plantuner' in
   psql session or specify 'shared_preload_libraries' option in
   postgresql.conf.
=# LOAD 'plantuner';
=# create table test(id int);
=# create index id_idx on test(id);
=# create index id_idx2 on test(id);
=# \d test
     Table "public.test"
 Column |  Type   | Modifiers
--------+---------+-----------
 id     | integer |
Indexes:
    "id_idx" btree (id)
    "id_idx2" btree (id)
=# explain select id from test where id=1;
                              QUERY PLAN
-----------------------------------------------------------------------
 Bitmap Heap Scan on test  (cost=4.34..15.03 rows=12 width=4)
   Recheck Cond: (id = 1)
   ->  Bitmap Index Scan on id_idx2  (cost=0.00..4.34 rows=12 width=0)
         Index Cond: (id = 1)
(4 rows)
=# set enable_seqscan=off;
=# set plantuner.disable_index='id_idx2';
=# explain select id from test where id=1;
                              QUERY PLAN
----------------------------------------------------------------------
 Bitmap Heap Scan on test  (cost=4.34..15.03 rows=12 width=4)
   Recheck Cond: (id = 1)
   ->  Bitmap Index Scan on id_idx  (cost=0.00..4.34 rows=12 width=0)
         Index Cond: (id = 1)
(4 rows)
=# set plantuner.disable_index='id_idx2,id_idx';
=# explain select id from test where id=1;
                               QUERY PLAN
-------------------------------------------------------------------------
 Seq Scan on test  (cost=10000000000.00..10000000040.00 rows=12 width=4)
   Filter: (id = 1)
(2 rows)
=# set plantuner.enable_index='id_idx';
=# explain select id from test where id=1;
                              QUERY PLAN
-----------------------------------------------------------------------
 Bitmap Heap Scan on test  (cost=4.34..15.03 rows=12 width=4)
   Recheck Cond: (id = 1)
   ->  Bitmap Index Scan on id_idx  (cost=0.00..4.34 rows=12 width=0)
         Index Cond: (id = 1)
(4 rows)


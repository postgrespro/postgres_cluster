# contrib/pglogical/Makefile

MODULE_big = pglogical
EXTENSION = pglogical
PGFILEDESC = "pglogical - logical replication"

DATA = pglogical--1.0.0.sql pglogical--1.0.1.sql pglogical--1.0.0--1.0.1.sql

OBJS = pglogical_apply.o pglogical_conflict.o pglogical_manager.o \
	   pglogical_node.o pglogical_proto.o pglogical_relcache.o \
	   pglogical.o pglogical_repset.o pglogical_rpc.o \
	   pglogical_functions.o pglogical_queue.o pglogical_fe.o \
	   pglogical_worker.o pglogical_hooks.o pglogical_sync.o

PG_CPPFLAGS = -I$(libpq_srcdir) -I$(top_srcdir)/contrib/pglogical_output
SHLIB_LINK = $(libpq)

REGRESS = preseed infofuncs init_fail init preseed_check basic extended toasted replication_set add_table matview bidirectional primary_key foreign_key functions copy drop

# In-tree builds only
subdir = contrib/pglogical
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk

# Disabled because these tests require "wal_level=logical", which
# typical installcheck users do not have (e.g. buildfarm clients).
@installcheck: ;

EXTRA_INSTALL += contrib/pglogical_output
EXTRA_REGRESS_OPTS += --temp-config $(top_srcdir)/contrib/pglogical/regress-postgresql.conf

.PHONY: pglogical_create_subscriber

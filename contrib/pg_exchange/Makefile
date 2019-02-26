# contrib/pg_exchange/Makefile

MODULE_big = pg_exchange
EXTENSION = pg_exchange
EXTVERSION = 0.1
PGFILEDESC = "pg_exchange - an exchange custom node and rules for the planner"
MODULES = pg_exchange
OBJS = pg_exchange.o exchange.o connection.o hooks.o common.o nodeDummyscan.o nodeDistPlanExec.o $(WIN32RES)

fdw_srcdir = $(top_srcdir)/contrib/postgres_fdw/

PG_CPPFLAGS = -I$(libpq_srcdir) -I$(fdw_srcdir) -L$(fdw_srcdir) 
SHLIB_LINK_INTERNAL = $(libpq)

DATA_built = $(EXTENSION)--$(EXTVERSION).sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
EXTRA_INSTALL = contrib/postgres_fdw
SHLIB_PREREQS = submake-libpq
subdir = contrib/pg_exchange
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

$(EXTENSION)--$(EXTVERSION).sql: init.sql
	cat $^ > $@

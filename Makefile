# contrib/pg_execplan/Makefile

MODULE_big = pg_execplan
EXTENSION = pg_execplan
EXTVERSION = 0.1
PGFILEDESC = "pg_execplan"
MODULES = pg_execplan
OBJS = pg_execplan.o $(WIN32RES)

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
subdir = contrib/pg_execplan
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
#include $(top_builddir)/contrib/postgres_fdw/Makefile
include $(top_srcdir)/contrib/contrib-global.mk
endif

$(EXTENSION)--$(EXTVERSION).sql: init.sql
	cat $^ > $@

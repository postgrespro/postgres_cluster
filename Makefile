# contrib/pg_repeater/Makefile

MODULE_big = pg_repeater
EXTENSION = pg_repeater
EXTVERSION = 0.1
PGFILEDESC = "pg_repeater"
MODULES = pg_repeater1
OBJS = pg_repeater.o $(WIN32RES)

fdw_srcdir = $(top_srcdir)/contrib/postgres_fdw/
execplan_srcdir = $(top_srcdir)/contrib/pg_execplan/

PG_CPPFLAGS = -I$(libpq_srcdir) -I$(fdw_srcdir) -L$(fdw_srcdir) -I$(execplan_srcdir) -L$(execplan_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

DATA_built = $(EXTENSION)--$(EXTVERSION).sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
EXTRA_INSTALL = contrib/postgres_fdw contrib/pg_execplan
SHLIB_PREREQS = submake-libpq
subdir = contrib/pg_repeater
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

$(EXTENSION)--$(EXTVERSION).sql: init.sql
	cat $^ > $@

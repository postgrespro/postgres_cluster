# contrib/pageinspect/Makefile

MODULE_big	= dump_stat
OBJS		= anyarray_elemtype.o $(WIN32RES)

EXTENSION = dump_stat
DATA = dump_stat--1.0.sql
PGFILEDESC = "dump_stat - move pg_statistic to new instance of PostgreSQL"

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/dump_stat
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# contrib/shared_ispell/Makefile

MODULE_big = shared_ispell
OBJS = src/shared_ispell.o

EXTENSION = shared_ispell
DATA = shared_ispell--1.1.0.sql

REGRESS = shared_ispell

EXTRA_REGRESS_OPTS=--temp-config=$(top_srcdir)/$(subdir)/postgresql.conf

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/shared_ispell
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

#installcheck:;

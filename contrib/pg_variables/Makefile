# contrib/pg_variables/Makefile

MODULE_big = pg_variables
OBJS = pg_variables.o pg_variables_record.o $(WIN32RES)

EXTENSION = pg_variables
DATA = pg_variables--1.0.sql
PGFILEDESC = "pg_variables - sessional variables"

REGRESS = pg_variables

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_variables
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

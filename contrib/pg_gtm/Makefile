MODULE_big = pg_dtm
OBJS = pg_dtm.o libdtm.o

EXTENSION = pg_dtm
DATA = pg_dtm--1.0.sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_dtm
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

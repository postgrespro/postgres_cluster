# pg_tsparser/Makefile

MODULE_big = pg_tsparser
OBJS = tsparser.o $(WIN32RES)

EXTENSION = pg_tsparser
DATA = pg_tsparser--1.0.sql
PGFILEDESC = "pg_tsparser - parser for text search"

REGRESS = pg_tsparser

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_tsparser
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

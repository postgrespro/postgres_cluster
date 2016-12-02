# pg_transfer/Makefile
MODULE_big = pg_transfer
OBJS = pg_transfer.o
EXTENSION = pg_transfer
DATA = pg_transfer--1.0.sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $( shell $( PG_CONFIG ) --pgxs )
include $(PGXS)
else
subdir = contrib/pg_transfer
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

MODULE_big = pg_tsdtm
OBJS = pg_tsdtm.o

EXTENSION = pg_tsdtm
DATA = pg_tsdtm--1.0.sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_tsdtm
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

check:
	env DESTDIR='$(abs_top_builddir)'/tmp_install make install
	$(prove_check)

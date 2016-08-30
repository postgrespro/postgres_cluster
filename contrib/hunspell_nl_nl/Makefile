EXTENSION = hunspell_nl_nl
DATA = hunspell_nl_nl--1.0.sql

DATA_TSEARCH = nl_nl.affix nl_nl.dict

REGRESS = hunspell_nl_nl

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hunspell_nl_nl
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

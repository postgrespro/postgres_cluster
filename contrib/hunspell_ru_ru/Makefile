EXTENSION = hunspell_ru_ru
DATA = hunspell_ru_ru--1.0.sql

DATA_TSEARCH = ru_ru.affix ru_ru.dict

REGRESS = hunspell_ru_ru

EXTRA_REGRESS_OPTS=--encoding=UTF8

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hunspell_ru_ru
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

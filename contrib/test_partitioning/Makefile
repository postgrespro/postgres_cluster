REGRESS = partition
EXTRA_REGRESS_OPTS=--temp-config=$(top_srcdir)/$(subdir)/pg_pathman.add
EXTRA_INSTALL=contrib/pg_pathman
ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/test_partitioning
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

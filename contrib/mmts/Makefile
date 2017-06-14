
EXTENSION = multimaster
DATA = multimaster--1.0.sql
OBJS = multimaster.o arbiter.o bytebuf.o bgwpool.o pglogical_output.o pglogical_proto.o pglogical_receiver.o pglogical_apply.o pglogical_hooks.o pglogical_config.o pglogical_relid_map.o ddd.o bkb.o spill.o
MODULE_big = multimaster

PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK = $(libpq)

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/mmts
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
.PHONY: all

EXTRA_INSTALL=contrib/mmts

all: multimaster.so



check: temp-install
	$(prove_check)


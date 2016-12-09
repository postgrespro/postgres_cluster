MODULE_big = multimaster
OBJS = multimaster.o arbiter.o bytebuf.o bgwpool.o pglogical_output.o pglogical_proto.o pglogical_receiver.o pglogical_apply.o pglogical_hooks.o pglogical_config.o pglogical_relid_map.o ddd.o bkb.o spill.o

EXTENSION = multimaster
DATA = multimaster--1.0.sql

.PHONY: all

all: multimaster.so

tests/dtmbench:
	make -C tests

PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK = $(libpq)

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/multimaster
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

check:
	env DESTDIR='$(abs_top_builddir)'/tmp_install make install
	$(prove_check)

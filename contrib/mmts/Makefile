MODULE_big = multimaster
OBJS = multimaster.o ../arbiter/lib/libarbiter.a ../arbiter/sockhub/libsockhub.a bytebuf.o bgwpool.o pglogical_output.o pglogical_proto.o pglogical_receiver.o pglogical_apply.o pglogical_hooks.o pglogical_config.o
#OBJS = multimaster.o pglogical_receiver.o decoder_raw.o libdtm.o bytebuf.o bgwpool.o sockhub/sockhub.o

override CPPFLAGS += -I../arbiter/api -I../arbiter/sockhub

EXTENSION = multimaster
DATA = multimaster--1.0.sql

.PHONY: all

all: multimaster.o multimaster.so

../arbiter/sockhub/libsockhub.a:
	make -C ../arbiter/sockhub

../arbiter/lib/libarbiter.a:
	make -C ../arbiter

PG_CPPFLAGS = -I$(libpq_srcdir) -DUSE_PGLOGICAL_OUTPUT
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


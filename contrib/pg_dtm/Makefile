MODULE_big = pg_dtm
OBJS = pg_dtm.o ../arbiter/lib/libarbiter.a ../arbiter/sockhub/libsockhub.a

override CPPFLAGS += -I../arbiter/api -I../arbiter/sockhub

.PHONY: all

all: pg_dtm.o pg_dtm.so

../arbiter/sockhub/libsockhub.a:
	make -C ../arbiter/sockhub

../arbiter/lib/libarbiter.a:
	make -C ../arbiter

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

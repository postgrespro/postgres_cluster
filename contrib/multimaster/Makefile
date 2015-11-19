MODULE_big = multimaster
OBJS = multimaster.o receiver_raw.o decoder_raw.o libdtm.o sockhub/sockhub.o

EXTENSION = multimaster
DATA = multimaster--1.0.sql

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


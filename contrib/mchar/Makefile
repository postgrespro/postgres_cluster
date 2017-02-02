MODULE_big = mchar
OBJS = mchar_io.o mchar_proc.o mchar_op.o mchar_recode.o \
	   mchar_like.o
EXTENSION = mchar
DATA = mchar--1.0.sql
DOCS = README.mchar
REGRESS = init mchar mvarchar mm like compat
PGFIELDDESC = "mchar - mchar type implementation"

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/mchar
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

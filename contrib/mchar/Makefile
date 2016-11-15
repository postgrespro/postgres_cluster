MODULE_big = mchar
OBJS = mchar_io.o mchar_proc.o mchar_op.o mchar_recode.o \
	   mchar_like.o
DATA_built = mchar.sql
DATA = uninstall_mchar.sql
DOCS = README.mchar
REGRESS = init mchar mvarchar mm like compat

PG_CPPFLAGS=-I/usr/local/include

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/mchar
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

ifeq ($(PORTNAME),win32)
ICUNAME=icuin
else
ICUNAME=icui18n
endif

SHLIB_LINK += -L/usr/local/lib -licuuc -l$(ICUNAME) -Wl,-R,'$$ORIGIN'

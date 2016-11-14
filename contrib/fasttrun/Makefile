MODULE_big = fasttrun
OBJS = fasttrun.o	
DATA_built = fasttrun.sql
DOCS = README.fasttrun
REGRESS = fasttrun

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/fasttrun
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

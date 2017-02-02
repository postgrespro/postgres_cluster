MODULE_big = online_analyze
OBJS = online_analyze.o     
#DATA_built = online_analyze.sql
DOCS = README.online_analyze
#REGRESS = online_analyze

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/online_analyze
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif


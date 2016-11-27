MODULE_big = fasttrun
OBJS = fasttrun.o $(WIN32RES)
EXTENSION = fasttrun
DATA = fasttrun--1.0.sql
DOCS = README.fasttrun
REGRESS = fasttrun
PGFIELDDESC = "fasttrun - functions to truncates the temporary table and doesn't grow pg_class size."

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/fasttrun
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

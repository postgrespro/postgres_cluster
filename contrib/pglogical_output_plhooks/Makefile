MODULES = pglogical_output_plhooks
EXTENSION = pglogical_output_plhooks
DATA = pglogical_output_plhooks--1.0.sql
DOCS = README.pglogical_output_plhooks

subdir = contrib/pglogical_output_plhooks
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk

# Allow the hook plugin to see the pglogical_output headers
# Necessary because !PGXS builds don't respect PG_CPPFLAGS
override CPPFLAGS := $(CPPFLAGS) -I$(top_srcdir)/contrib/pglogical_output

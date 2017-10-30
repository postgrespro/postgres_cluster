# contrib/postgres_fdw/Makefile

MODULE_big = postgres_fdw
OBJS = postgres_fdw.o option.o deparse.o connection.o shippable.o $(WIN32RES)
PGFILEDESC = "postgres_fdw - foreign data wrapper for PostgreSQL"

PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK = $(libpq)

EXTENSION = postgres_fdw
DATA = postgres_fdw--1.0.sql

REGRESS = postgres_fdw

# EXTRA_INSTALL += contrib/pg_tsdtm

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
SHLIB_PREREQS = submake-libpq
subdir = contrib/postgres_fdw
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

# Poor's man overload of already defined target: just copy it
check: submake $(REGRESS_PREP)
	# $(pg_regress_check) $(REGRESS_OPTS) $(REGRESS)
	env DESTDIR='$(abs_top_builddir)'/tmp_install make install
	$(prove_check)

# contrib/postgres_fdw/Makefile

MODULE_big = postgres_fdw
OBJS = postgres_fdw.o option.o deparse.o connection.o shippable.o $(WIN32RES)
PGFILEDESC = "postgres_fdw - foreign data wrapper for PostgreSQL"

PG_CPPFLAGS = -I$(libpq_srcdir)
SHLIB_LINK_INTERNAL = $(libpq)

EXTENSION = postgres_fdw
DATA = postgres_fdw--1.0.sql

REGRESS = postgres_fdw

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

# Global makefile will do temp-install for 'check'. Since REGRESS is defined,
# PGXS (included from contrib-global.mk or directly) will care to add
# postgres_fdw to it as EXTRA_INSTALL and build pg_regress. It will also
# actually run pg_regress, so the only thing left is tap tests.
check: tapcheck

tapcheck: temp-install
	$(prove_check)

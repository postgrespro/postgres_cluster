#
# pg_repack: lib/Makefile
#
#  Portions Copyright (c) 2008-2012, NIPPON TELEGRAPH AND TELEPHONE CORPORATION
#  Portions Copyright (c) 2011, Itagaki Takahiro
#  Portions Copyright (c) 2012-2015, The Reorg Development Team
#

EXTENSION = pg_repack
MODULE_big = $(EXTENSION)

OBJS = repack.o pgut/pgut-be.o pgut/pgut-spi.o
ifdef USE_PGXS
PG_CONFIG ?= pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
VERSION := $(shell $(PG_CONFIG) --version | awk '{print $$2}')
include $(PGXS)
else
subdir= contrib/pg_repack/lib
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif


# version as a number, e.g. 9.1.4 -> 901
INTVERSION := $(shell echo $$(($$(echo $(VERSION) | sed 's/\([[:digit:]]\{1,\}\)\.\([[:digit:]]\{1,\}\).*/\1*100+\2/'))))


SHLIB_EXPORTS = exports.txt

# The version number of the program. It should be the same of the library.
REPACK_VERSION = $(shell grep '"version":' ../META.json | head -1 \
	| sed -e 's/[ 	]*"version":[ 	]*"\(.*\)",/\1/')

PG_CPPFLAGS = -DREPACK_VERSION=$(REPACK_VERSION)

# Support CREATE EXTENSION for PG >= 9.1 and a simple sql script for PG < 9.1
ifeq ($(shell echo $$(($(INTVERSION) >= 901))),1)
DATA_built = pg_repack--$(REPACK_VERSION).sql pg_repack.control
else
DATA_built = pg_repack.sql
DATA = uninstall_pg_repack.sql
endif

# remove dependency on libxml2, libxslt, and libpam.
# XXX: find a better way to make sure we are linking with libraries
# from pg_config which we actually need.
LIBS := $(filter-out -lxml2, $(LIBS))
LIBS := $(filter-out -lxslt, $(LIBS))
LIBS := $(filter-out -lpam, $(LIBS))

pg_repack.sql: pg_repack.sql.in
	echo "BEGIN;" > $@; \
	sed 's,MODULE_PATHNAME,$$libdir/$(MODULE_big),g' $< \
	| sed 's,REPACK_VERSION,$(REPACK_VERSION),g' >> $@; \
	echo "COMMIT;" >> $@;

pg_repack--$(REPACK_VERSION).sql: pg_repack.sql.in
	sed 's,REPACK_VERSION,$(REPACK_VERSION),g' $< > $@;

pg_repack.control: pg_repack.control.in
	sed 's,REPACK_VERSION,$(REPACK_VERSION),g' $< > $@

#
# pg_repack: Makefile
#
#  Portions Copyright (c) 2008-2011, NIPPON TELEGRAPH AND TELEPHONE CORPORATION
#  Portions Copyright (c) 2011, Itagaki Takahiro
#  Portions Copyright (c) 2012-2015, The Reorg Development Team
#

ifdef USE_PGXS
PG_CONFIG ?= pg_config
# Pull out PostgreSQL version number from pg_config
VERSION := $(shell $(PG_CONFIG) --version | awk '{print $$2}')
ifeq ("$(VERSION)","")
$(error pg_config not found)
endif
else
subdir= contrib/pg_repack
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
EXTENSION = pg_repack


SUBDIRS = bin lib regress

all install installdirs uninstall distprep clean distclean maintainer-clean debug:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@ || exit; \
	done

# We'd like check operations to run all the subtests before failing.
check installcheck:
	CHECKERR=0; for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@ || CHECKERR=$$?; \
	done; \
	exit $$CHECKERR

# Prepare the package for PGXN submission
package: dist dist/$(EXTENSION)-$(EXTVERSION).zip

dist:
	mkdir -p dist

dist/$(EXTENSION)-$(EXTVERSION).zip:
	git archive --format zip --prefix=$(EXTENSION)-$(EXTVERSION)/ --output $@ master

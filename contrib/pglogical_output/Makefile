MODULE_big = pglogical_output
PGFILEDESC = "pglogical_output - logical replication output plugin"

OBJS = pglogical_output.o pglogical_hooks.o pglogical_config.o \
	   pglogical_proto.o pglogical_proto_native.o \
	   pglogical_proto_json.o

REGRESS = pre_clean params_native basic_native hooks_native basic_json hooks_json encoding_json


subdir = contrib/pglogical_output
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk

# 'make installcheck' disabled when building in-tree because these tests
# require "wal_level=logical", which typical installcheck users do not have
# (e.g. buildfarm clients).
installcheck:
	;

EXTRA_INSTALL += contrib/pglogical_output_plhooks
EXTRA_REGRESS_OPTS += --temp-config=./regression.conf

install: all
	$(MKDIR_P) '$(DESTDIR)$(includedir)'/pglogical_output
	$(INSTALL_DATA) pglogical_output/hooks.h '$(DESTDIR)$(includedir)'/pglogical_output

# contrib/sr_plan/Makefile

MODULE_big = sr_plan
OBJS = sr_plan.o serialize.o deserialize.o $(WIN32RES)

EXTENSION = sr_plan
DATA = sr_plan--1.0.sql sr_plan--unpackaged--1.0.sql
PGFILEDESC = "sr_plan - save and read plan"

REGRESS = sr_plan

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/sr_plan
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif


genparser:
#	test -d sr_plan_env ||  
	python gen_parser.py nodes.h `pg_config --includedir-server`

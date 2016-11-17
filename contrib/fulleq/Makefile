MODULE_big = fulleq
OBJS = fulleq.o	
EXTENSION = fulleq
DATA = fulleq--1.0.sql
DOCS = README.fulleq
REGRESS = fulleq
PGFIELDDESC = "fulleq - introduce operator == which returns true when operands are equal or both are nulls."

ARGTYPE = bool bytea char name int8 int2 int2vector int4 text \
	oid xid cid oidvector float4 float8 abstime reltime macaddr \
	inet cidr varchar date time timestamp timestamptz \
	interval timetz

#EXTRA_CLEAN = fulleq--1.0.sql

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/fulleq
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

fulleq--1.0.sql:	fulleq--1.0.sql.in.in
	echo '\echo Use "CREATE EXTENSION fulleq" to load this file. \quit' >	$@
	echo 'SET search_path = public;' >> $@
	for type in	$(ARGTYPE);	\
	do	\
		sed -e "s/ARGTYPE/$$type/g" < $< >> $@;	\
	done


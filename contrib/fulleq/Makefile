MODULE_big = fulleq
OBJS = fulleq.o	
DATA_built = fulleq.sql
DOCS = README.fulleq
REGRESS = fulleq

ARGTYPE = bool bytea char name int8 int2 int2vector int4 text \
	oid xid cid oidvector float4 float8 abstime reltime macaddr \
	inet cidr varchar date time timestamp timestamptz \
	interval timetz

EXTRA_CLEAN = fulleq.sql.in

ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/fulleq
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

fulleq.sql.in:	fulleq.sql.in.in
	echo 'BEGIN;' >	$@ 
	echo 'SET search_path = public;' >> $@
	for type in	$(ARGTYPE);	\
	do	\
		sed -e "s/ARGTYPE/$$type/g" < $< >> $@;	\
	done
	echo 'COMMIT;' >>  $@


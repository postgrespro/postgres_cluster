# contrib/Makefile

subdir = contrib
top_builddir = ..
include $(top_builddir)/src/Makefile.global

SUBDIRS = \
		adminpack	\
		auth_delay	\
		auto_explain	\
		bloom		\
		btree_gin	\
		btree_gist	\
		chkpass		\
		citext		\
		cube		\
		dblink		\
		dict_int	\
		dict_xsyn	\
		earthdistance	\
		file_fdw	\
		fuzzystrmatch	\
		hstore		\
		intagg		\
		intarray	\
		isn		\
		lo		\
		ltree		\
		oid2name	\
		pageinspect	\
		passwordcheck	\
		pgpro_scheduler \
		pg_buffercache	\
		pg_freespacemap \
		pg_prewarm	\
		pg_query_state \
		pg_standby	\
		pg_stat_statements \
		pg_transfer	\
		pg_trgm		\
		pgcrypto	\
		pgrowlocks	\
		pgstattuple	\
		pg_visibility	\
		postgres_fdw	\
		rum		\
		seg		\
		spi		\
		tablefunc	\
		tcn		\
		test_decoding	\
		tsm_system_rows \
		tsm_system_time \
		tsearch2	\
		unaccent	\
		vacuumlo	\
		dump_stat	\
		hunspell_en_us	\
		hunspell_fr	\
		hunspell_nl_nl	\
		hunspell_ru_ru	\
		pg_variables	\
		jsquery		\
		sr_plan		\
		pg_probackup	\
		pg_pathman	\
		shared_ispell   \
		pg_hint_plan	\
		fulleq          \
		fasttrun        \
		online_analyze  \
		plantuner

ifeq ($(with_openssl),yes)
SUBDIRS += sslinfo
else
ALWAYS_SUBDIRS += sslinfo
endif

ifneq ($(with_uuid),no)
SUBDIRS += uuid-ossp
else
ALWAYS_SUBDIRS += uuid-ossp
endif

ifeq ($(with_libxml),yes)
SUBDIRS += xml2
else
ALWAYS_SUBDIRS += xml2
endif

ifeq ($(with_selinux),yes)
SUBDIRS += sepgsql
else
ALWAYS_SUBDIRS += sepgsql
endif

ifeq ($(with_perl),yes)
SUBDIRS += hstore_plperl
else
ALWAYS_SUBDIRS += hstore_plperl
endif

ifeq ($(with_python),yes)
SUBDIRS += hstore_plpython ltree_plpython
else
ALWAYS_SUBDIRS += hstore_plpython ltree_plpython
endif

ifeq ($(with_icu),yes)
SUBDIRS += mchar
else
ALWAYS_SUBDIRS += mchar
endif

# Missing:
#		start-scripts	\ (does not have a makefile)


$(recurse)
$(recurse_always)

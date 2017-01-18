MODULE_big = pgpro_scheduler
OBJS = src/pgpro_scheduler.o src/cron_string.o src/sched_manager_poll.o \
	src/char_array.o src/scheduler_spi_utils.o src/scheduler_manager.o \
	src/bit_array.o src/scheduler_job.o src/memutils.o \
	src/scheduler_executor.o \
	$(WIN32RES)
EXTENSION = pgpro_scheduler
DATA = pgpro_scheduler--1.0.sql
REGRESS	= install_pgpro_scheduler cron_string
#REGRESS_OPTS = --user=postgres
EXTRA_REGRESS_OPTS=--temp-config=$(top_srcdir)/$(subdir)/conf.add

ifdef USE_PGXS
	PG_CONFIG = pg_config
	PGXS := $(shell $(PG_CONFIG) --pgxs)
	include $(PGXS)
else
	subdir = contrib/pgpro_scheduler
	top_builddir = ../..
	include $(top_builddir)/src/Makefile.global
	include $(top_srcdir)/contrib/contrib-global.mk
endif

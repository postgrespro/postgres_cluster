MODULE_big = raftable
OBJS = raftable.o worker.o state.o blockmem.o
EXTENSION = raftable
DATA = raftable--1.0.sql

raftable.so: raft/lib/libraft.a

raft/lib/libraft.a:
	make -C raft

EXTRA_INSTALL = contrib/raftable

RAFT_PREFIX = raft
override LDFLAGS += -L$(RAFT_PREFIX)/lib -Wl,-whole-archive -lraft -Wl,-no-whole-archive
override CFLAGS += -Wfatal-errors
override CPPFLAGS += -I$(RAFT_PREFIX)/include

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/raftable
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

check:
	$(prove_check)

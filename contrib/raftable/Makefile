MODULE_big = raftable
OBJS = raftable.o worker.o state.o blockmem.o raft/obj/raft.o raft/obj/util.o
EXTENSION = raftable
DATA = raftable--1.0.sql

.PHONY: all

all: raftable.so

raft/obj/raft.o raft/obj/util.o:
	make -C raft obj/raft.o obj/util.o

EXTRA_INSTALL = contrib/raftable

override CFLAGS += -Wfatal-errors -Wall
override CPPFLAGS += -Iraft/include

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

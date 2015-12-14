MODULE_big = pg_dtm
OBJS = pg_dtm.o

EXTENSION = pg_dtm
DATA = pg_dtm--1.0.sql

ifndef PG_CONFIG
PG_CONFIG = pg_config
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


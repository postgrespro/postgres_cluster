MODULE_big = multimaster
OBJS = multimaster.o receiver_raw decorer_raw libdtm.o sockhub/sockhub.o

EXTENSION = multimaster
DATA = multimaster--1.0.sql

ifndef PG_CONFIG
PG_CONFIG = pg_config
endif

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

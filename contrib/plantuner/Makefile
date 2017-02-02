MODULE_big = plantuner
DOCS = README.plantuner
REGRESS = plantuner
OBJS=plantuner.o

ifdef USE_PGXS
PGXS = $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/plantuner
top_builddir = ../..
include $(top_builddir)/src/Makefile.global

include $(top_srcdir)/contrib/contrib-global.mk
endif

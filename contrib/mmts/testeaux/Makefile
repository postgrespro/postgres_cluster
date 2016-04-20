
.PHONY: all


subdir = contrib/multimaster/testeaux
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk

start:
	echo "Installing postgres to SRCDIR/tmp_install"
	cd $(abs_top_builddir) && env DESTDIR='$(abs_top_builddir)'/tmp_install make install > /dev/null
	echo "Installing raftable to SRCDIR/tmp_install"
	cd $(abs_top_builddir)/contrib/raftable && env DESTDIR='$(abs_top_builddir)'/tmp_install make install
	echo "Installing mmts to SRCDIR/tmp_install"
	cd $(abs_top_builddir)/contrib/mmts && env DESTDIR='$(abs_top_builddir)'/tmp_install make install
	rm -rf $(CURDIR)/tmp_check/log
	echo "Ininializing new postgres cluster in contrib/mmts/tmp_install"
	cd $(srcdir) && TESTDIR='$(CURDIR)' PERL5LIB=$(abs_top_builddir)/src/test/perl:$(PERL5LIB) $(with_temp_install) PGPORT='6$(DEF_PGPORT)' PG_REGRESS='$(CURDIR)/$(top_builddir)/src/test/regress/pg_regress' perl run.pl

stop:
	echo "Stopping all instances in tmp_install"
	for DIR in ./tmp_check/data_node*; do \
		pg_ctl stop -m immediate -D $$DIR/pgdata; \
		rm -rf $$DIR; \
	done

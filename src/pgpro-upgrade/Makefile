#-------------------------------------------------------------------------
#
# Makefile for src/pgpro-upgrade (Postgres Pro specific upgrade scripts)
#
# Copyright (c) 2016, Postgres Professional
#
# src/pgpro-upgrade/Makefile
#
#-------------------------------------------------------------------------

subdir = src/pgpro-upgrade
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
srcdir=$(top_srcdir)/$(subdir)

all:
	$(PERL) $(srcdir)/setver.pl $(top_srcdir) $(top_builddir)
	chmod 0755 pgpro_upgrade

install: installdirs 
	$(INSTALL_PROGRAM) pgpro_upgrade '$(DESTDIR)$(bindir)/pgpro_upgrade'
	$(INSTALL_DATA) $(srcdir)/*.sql $(srcdir)/*.test '$(DESTDIR)$(datadir)/pgpro-upgrade'
installdirs:
	$(MKDIR_P) '$(DESTDIR)$(bindir)'
	$(MKDIR_P) '$(DESTDIR)$(datadir)/pgpro-upgrade'

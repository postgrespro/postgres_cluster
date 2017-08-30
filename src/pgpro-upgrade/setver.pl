#!/usr/bin/perl
use Cwd qw(getcwd abs_path);
my $major_version;
my $catversion;
my $curdir = abs_path(getcwd()."/../..");
my $top_srcdir = $ARGV[0] || $curdir;
my $top_builddir = $ARGV[1] || $curdir;
open F,"$top_builddir/src/include/pg_config.h";
while (<F>) {
	$major_version = $1 if  /#define PG_MAJORVERSION "(.*)"/;
}
close F;
open F,"$top_srcdir/src/include/catalog/catversion.h" or die "catversion.h $!\n";
while (<F>) {
	$catversion = $1  if /#define CATALOG_VERSION_NO\s+(\S+)/;
}
close F;
if (-f "pgpro_upgrade") {
	unlink("pgpro_upgrade.bak") if -f "pgpro_upgrade.bak";
	rename("pgpro_upgrade","pgpro_upgrade.bak");
	open IN,"pgpro_upgrade.bak"
} else {
	open IN,"$top_srcdir/src/pgpro-upgrade/pgpro_upgrade";
}
open OUT,">","pgpro_upgrade";
while (<IN>) {
	s/^CATALOG_VERSION_NO=.*$/CATALOG_VERSION_NO=$catversion/;
	s/^MAJORVER=.*$/MAJORVER=$major_version/;
	print OUT $_;
}
close IN;
close OUT;

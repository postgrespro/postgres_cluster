#!/usr/bin/perl

use Combineaux;

sub list_modules
{
	my $dir = shift;
	my @modules = ();

	opendir DIR, $dir or die "Cannot open directory: $!";

	while (my $file = readdir(DIR)) {
		# Use a regular expression to ignore files beginning with a period
		next unless ($file =~ m/\.pm$/);
		push @modules, substr($file, 0, -3);
	}

	closedir DIR;

	return @modules;
}

my @stresses = list_modules('stress');
my @troubles = list_modules('trouble');

Combineaux::combine(\@stresses, \@troubles);

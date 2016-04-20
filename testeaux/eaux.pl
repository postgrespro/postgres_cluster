#!/usr/bin/perl

use Testeaux;

my @workloads = qw(
	bank-transfers
	pgbench-default
);
my @troubles = qw(
	split-brain
	time-shift
);

Combineaux::combine(\@workloads, \@troubles);

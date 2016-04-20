#!/usr/bin/perl

use Testeaux;

Combineaux::combine(
	['bank-transfers', 'pgbench-default'],
	['split-brain', 'time-shift'],
)

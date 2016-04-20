package Stresseaux;

my %running = ();

sub start
{
	my ($id, $workload, $cluster) = @_;
	print("start stress $id: workload $workload, cluster $cluster\n");

	if (exists $running{$id})
	{
		print("cannot start stress $id: that id has already been taken\n");
		return 0;
	}

	# FIXME: implement 'stress'/'workload' objects
	$running{$id} = {hello => 'world'};

	# FIXME: implement

	return 1;
}

sub stop
{
	my $id = shift;
	print("stop stress $id\n");

	my $stress = delete $running{$id};

	if (!defined $stress)
	{
		print("cannot stop stress $id: that id is not running\n");
		return 0;
	}

	# FIXME: implement

	return 1;
}

1;

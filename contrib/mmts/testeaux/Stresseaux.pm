package Stresseaux;

my %running = ();

sub start
{
	my ($id, $stressname, $cluster, @args) = @_;

	if (exists $running{$id})
	{
		print("cannot start stress $stressname as $id: that id has already been taken\n");
		return 0;
	}

	require "stress/$stressname.pm";
	$running{$id} = "stress::$stressname"->start($id, $cluster, @args);

	return 1;
}

sub stop
{
	my $id = shift;

	my $stress = delete $running{$id};

	if (!defined $stress)
	{
		print("cannot stop stress $id: that id is not running\n");
		return 0;
	}

	$stress->stop();

	return 1;
}

1;

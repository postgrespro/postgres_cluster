package Testeaux;

use Stresseaux;

package Combineaux
{
	sub combine
	{
		my ($workloads, $troubles) = @_;

		my $cluster = Starteaux->deploy('lxc');

		foreach my $workload (@$workloads)
		{
			foreach my $trouble (@$troubles)
			{
				print("--- $workload vs. $trouble ---\n");
				my $id = "$workload+$trouble";
				Stresseaux::start($id, $workload, $cluster) || die "stress wouldn't start";
				sleep(1); # FIXME: will this work?
				Troubleaux::cause($cluster, $trouble);
				sleep(1); # FIXME: will this work?
				Stresseaux::stop($id) || die "stress wouldn't stop";
				Troubleaux::fix($cluster);
			}
		}

		$cluster->destroy();
	}
}

package Starteaux
{
	sub deploy
	{
		my ($class, $driver, @args) = @_;
		my $self = {};
		print("deploy cluster using driver $driver\n");
		# fixme: implement
		return bless $self, 'Starteaux';
	}

	sub up
	{
		my ($self, $id) = @_;
		print("up node $id\n");
		# FIXME: implement
	}

	sub down
	{
		my ($self, $id) = @_;
		print("down node $id\n");
		# FIXME: implement
	}

	sub drop
	{
		my ($self, $src, $dst, $ratio) = @_;
		print("drop $ratio packets from $src to $dst\n");
		# FIXME: implement
	}

	sub delay
	{
		my ($self, $src, $dst, $msec) = @_;
		print("delay packets from $src to $dst by $msec msec\n");
		# FIXME: implement
	}

	sub destroy
	{
		my ($self) = @_;
		print("destroy cluster $cluster\n");
		# FIXME: implement
	}
}

package Troubleaux
{
	sub cause
	{
		my ($cluster, $trouble) = @_;
		print("cause $trouble in cluster $cluster\n");
		# fixme: implement
	}

	sub fix
	{
		my ($cluster) = @_;
		print("fix cluster $cluster\n");
		# fixme: implement
	}
}

1;

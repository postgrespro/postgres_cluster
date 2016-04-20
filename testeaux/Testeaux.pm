package Testeaux;

package combineaux
{
	sub combine
	{
		my ($workloads, $troubles) = @_;

		my $cluster = starteaux->deploy('lxc');

		foreach my $workload (@$workloads)
		{
			foreach my $trouble (@$troubles)
			{
				print("run workload $workload during trouble $trouble\n");
				# FIXME: generate proper id instead of 'hello'
				stresseaux::start('hello', $workload, $cluster);
				# FIXME: add a time gap here
				troubleaux::cause($cluster, $trouble);
				# FIXME: add a time gap here
				stresseaux::stop('hello');
				troubleaux::fix($cluster);
			}
		}
	}
}

package stresseaux
{
	sub start
	{
		my ($id, $workload, $cluster) = @_;
		print("start stress $id: workload $workload, cluster $cluster\n");
		# fixme: implement
	}

	sub stop
	{
		my $id = shift;
		print("stop stress $id\n");
		# FIXME: implement
	}
}

package starteaux
{
	sub deploy
	{
		my ($class, $driver, @args) = @_;
		my $self = {};
		print("deploy cluster using driver $driver\n");
		# fixme: implement
		return bless $self, 'starteaux';
	}

	sub up
	{
		my ($self, $id) = @_;
		print("up node $id\n");
		# FIXME: implement
	}

	sub down
	{
		my ($self, $id = @_;
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
}

1;

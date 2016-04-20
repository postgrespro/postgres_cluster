package Testeaux;

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
				print("run workload $workload during trouble $trouble\n");
				# FIXME: generate proper id instead of 'hello'
				Stresseaux::start('hello', $workload, $cluster);
				sleep(1); # FIXME: will this work?
				Troubleaux::cause($cluster, $trouble);
				sleep(1); # FIXME: will this work?
				Stresseaux::stop('hello');
				Troubleaux::fix($cluster);
			}
		}

		$cluster->destroy();
	}
}

package Stresseaux
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

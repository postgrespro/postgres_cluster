package Starteaux;

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

1;

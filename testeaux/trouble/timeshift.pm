package trouble::timeshift;

sub cause
{
	my $class = shift;
	my $self = {};
	print("cause $class with args " . join(',', @args) . "\n");
	return bless $self, $class;
}

sub fix
{
	my $self = shift;
	print("fix $self\n");
}

1;

package trouble::splitbrain;

sub cause
{
	my ($class, @args) = @_;
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

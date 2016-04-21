package stress::banktransfer;

sub start
{
	my ($class, @args) = @_;
	my $self = {};
	print("start $class with args " . join(',', @args) . "\n");
	return bless $self, $class;
}

sub stop
{
	my $self = shift;
	print("stop $self\n");
}

1;

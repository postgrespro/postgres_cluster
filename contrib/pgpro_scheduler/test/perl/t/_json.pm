package t;

use strict;

sub json
{
	my $data = shift;

	return _element($data);
}

sub _escape
{
	my $str = shift;

	$str =~ s/"/\\"/g;
	return $str;
}

sub _element
{
	my $data = shift;

	if(ref($data) eq 'HASH')
	{
		my @out;
		foreach my $name (keys %$data)
		{
			push @out, q["]._escape($name).q[":]._element($data->{$name});
		}

		return "{".join(', ', @out)."}";
	}
	elsif(ref($data) eq 'ARRAY')
	{
		return "[".join(', ', map { _element($_) } @$data)."]";
	}
	else
	{
		return q["]._escape($data).q["];
	}
}

1;

my @caused = ();

package Troubleaux
{
	sub cause
	{
		my $cluster = shift;
		my $troublename = shift;
		my @args = @_;

		require "trouble/$troublename.pm";
		push @caused, "trouble::$troublename"->cause(@args);
	}

	sub fix
	{
		my ($cluster) = @_;

		while (@caused)
		{
			my $trouble = pop @caused;
			$trouble->fix();
		}
	}
}

1;

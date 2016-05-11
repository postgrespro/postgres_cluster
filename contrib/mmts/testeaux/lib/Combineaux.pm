package Combineaux;

use Troubleaux;
use Stresseaux;
use Starteaux;

sub combine
{
	my ($stresses, $troubles) = @_;

	my $cluster = Starteaux->deploy('lxc');

	foreach my $stress (@$stresses)
	{
		foreach my $trouble (@$troubles)
		{
			print("--- $stress vs. $trouble ---\n");
			my $id = "$stress+$trouble";
			Stresseaux::start($id, $stress, $cluster) || die "stress wouldn't start";
			sleep(1); # FIXME: will this work?
			Troubleaux::cause($cluster, $trouble);
			sleep(1); # FIXME: will this work?
			Stresseaux::stop($id) || die "stress wouldn't stop";
			Troubleaux::fix($cluster);
		}
	}

	$cluster->destroy();
}

1;

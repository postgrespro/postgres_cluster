package RemoteNode;

use strict;
use warnings;
use Net::OpenSSH;

sub new
{
	my ($class, $name, $sshopts) = @_;
	my ($node_id) = $name =~ /(\d+)/;

	print "### Creating node $name.\n";

	my $self = {
		_name    => $name,
		_id      => $node_id + 1,
		_port    => $sshopts->{Port},
		_host    => $sshopts->{HostName},
		_user    => $sshopts->{User},
		_keypath => $sshopts->{IdentityFile} =~ /"([^"]*)"/,
		_pgdata  => "/home/$sshopts->{User}/pg_cluster/data_5432",
		_pgbin   => "/home/$sshopts->{User}/pg_cluster/install/bin",
	};

	bless $self, $class;

	# kill postgres here to ensure
	# predictable initial state.
	$self->execute("pkill -9 postgres || true");

	return $self;
}

sub connstr
{
	my ($self, $dbname) = @_;

	"host=$self->{_host} dbname=$dbname";
}

sub execute
{
	my ($self, $cmd) = @_;

	# XXX: reuse connection
	my $ssh = Net::OpenSSH->new(
		host => $self->{_host},
		port => $self->{_port},
		user => $self->{_user},
		key_path => $self->{_keypath},
		master_opts => [-o => "StrictHostKeyChecking=no"]
	);

	print "# running \"$cmd\":\n";
	
	my $output = $ssh->capture($cmd);

	$ssh->error and
      warn "operation didn't complete successfully: ". $ssh->error;

	# XXX: tab and colorize output
	print $output;
	print "---\n";

	return $?;
}

sub init
{
	my ($self, %params) = @_;
	my $pgbin = $self->{_pgbin}; 
	my $pgdata = $self->{_pgdata};

	$self->execute("rm -rf $pgdata");
	$self->execute("env LANG=C LC_ALL=C $pgbin/initdb -D $pgdata -A trust -N");
	
	$self->append_conf("postgresql.conf", "fsync = off");
	$self->append_conf("pg_hba.conf", "host all all 0.0.0.0/0 trust");
}

sub start
{
	my ($self) = @_;
	my $pgbin = $self->{_pgbin}; 
	my $pgdata = $self->{_pgdata};

	$self->execute("ulimit -c unlimited && $pgbin/pg_ctl -w -D $pgdata -l $pgdata/log start");
}

sub stop
{
	my ($self, $mode) = @_;
	my $pgbin = $self->{_pgbin}; 
	my $pgdata = $self->{_pgdata};

	$self->execute("$pgbin/pg_ctl -w -D $pgdata -m $mode stop");
}

sub restart
{
	my ($self) = @_;
	my $pgbin = $self->{_pgbin}; 
	my $pgdata = $self->{_pgdata};

	$self->execute("$pgbin/pg_ctl -w -D $pgdata  -l $pgdata/log restart");
}

sub append_conf
{
	my ($self, $fname, $conf_str) = @_;
	my $pgdata = $self->{_pgdata};
	my $cmd = "cat <<- EOF >> $pgdata/$fname \n $conf_str \nEOF\n";

	$self->execute($cmd);
}

1;

package RemoteNode;

use strict;
use warnings;
use Net::OpenSSH;

sub new
{
	my ($class, $name, $sshopts) = @_;

	my $self = {
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
	
	$self->execute("echo 'fsync = off' >> $pgdata/postgresql.conf");
	$self->execute("echo 'host all all 0.0.0.0/0 trust' >> $pgdata/pg_hba.conf");
}

sub start
{
	my ($self) = @_;
	my $pgbin = $self->{_pgbin}; 
	my $pgdata = $self->{_pgdata};

	$self->execute("$pgbin/pg_ctl -w -D $pgdata -l $pgdata/log start");
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

	$self->execute("$pgbin/pg_ctl -w -D $pgdata restart");
}

sub append_conf
{

}

sub psql
{

}

# XXX: test
my $node = new RemoteNode('node0', {
          'Port' => '2200',
          'IdentityFile' => '"/Users/stas/code/postgres_cluster/contrib/mmts/testeaux/.vagrant/machines/node1/virtualbox/private_key"',
          'IdentitiesOnly' => 'yes',
          'LogLevel' => 'FATAL',
          'PasswordAuthentication' => 'no',
          'StrictHostKeyChecking' => 'no',
          'HostName' => '127.0.0.1',
          'User' => 'vagrant',
          'UserKnownHostsFile' => '/dev/null'
        });

$node->execute("ls -ls");
$node->init;
$node->start;



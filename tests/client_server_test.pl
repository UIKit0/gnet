#!/usr/bin/perl -w

# client_server_test.pl: Runs echoclient/echoserver pairs.
#
# Also can do: client_server_test.pl <server> <client> <num clients>
#   to run a specific test.

my $bin_dir = '../examples';
my $port = 8174;
my $debug = 0;
my $testfile = 'testfile';
my $pause = 3;

my @pids = ();

my $failed = 0;


$| = 1;
$SIG{'INT'} = 'on_sigint';

system ("rm -f .client* .server* core*");


if ($#ARGV == 2)
{
    test ($ARGV[0], $ARGV[1], $ARGV[2]);
    exit $failed;
}
elsif ($#ARGV != -1)
{
    usage();
}

test ("echoserver", 	    "echoclient", 1);
test ("echoserver", 	    "echoclient", 2);
test ("echoserver", 	    "echoclient", 16);
test ("echoserver", 	    "echoclient", 32);
	
test ("echoserver", 	    "echoclient-async", 1);
test ("echoserver", 	    "echoclient-async", 2);
test ("echoserver", 	    "echoclient-async", 16);
test ("echoserver", 	    "echoclient-async", 32);
	
test ("echoserver", 	    "echoclient-gconn", 1);
test ("echoserver", 	    "echoclient-gconn", 2);
test ("echoserver", 	    "echoclient-gconn", 16);
test ("echoserver", 	    "echoclient-gconn", 32);


test ("echoserver-async",   "echoclient", 1);
test ("echoserver-async",   "echoclient", 2);
test ("echoserver-async",   "echoclient", 16);
test ("echoserver-async",   "echoclient", 32);
  
test ("echoserver-async",   "echoclient-async", 1);
test ("echoserver-async",   "echoclient-async", 2);
test ("echoserver-async",   "echoclient-async", 16);
test ("echoserver-async",   "echoclient-async", 32);
  
test ("echoserver-async",   "echoclient-gconn", 1);
test ("echoserver-async",   "echoclient-gconn", 2);
test ("echoserver-async",   "echoclient-gconn", 16);
test ("echoserver-async",   "echoclient-gconn", 32);


test ("echoserver-gserver", "echoclient", 1);
test ("echoserver-gserver", "echoclient", 2);
test ("echoserver-gserver", "echoclient", 16);
test ("echoserver-gserver", "echoclient", 32);

test ("echoserver-gserver", "echoclient-async", 1);
test ("echoserver-gserver", "echoclient-async", 2);
test ("echoserver-gserver", "echoclient-async", 16);
test ("echoserver-gserver", "echoclient-async", 32);

test ("echoserver-gserver", "echoclient-gconn", 1);
test ("echoserver-gserver", "echoclient-gconn", 2);
test ("echoserver-gserver", "echoclient-gconn", 16);
test ("echoserver-gserver", "echoclient-gconn", 32);


test ("echoserver-udp",     "echoclient-udp", 1);
test ("echoserver-udp",     "echoclient-udp", 2);
test ("echoserver-udp",     "echoclient-udp", 16);
test ("echoserver-udp",     "echoclient-udp", 32);

exit $failed;


########################################

sub usage
{
    print STDERR ("Usage: $0\n");
    print STDERR ("          - Runs all tests\n");
    print STDERR ("       $0 <server> <client> <num runs>\n");
    print STDERR ("          - Runs a specific test\n");
    exit 1;
}

sub test
{
    my $server = $_[0];
    my $client = $_[1];
    my $num_clients = $_[2];
    my $tempfile = "$server.$client.$num_clients";

    my $fail = 0;

    printf "%-40s ", "$server $client $num_clients";

    system ("rm -f core*");

    sleep($pause);

    # Launch server
    server ($server, $tempfile);
    sleep (2);

    # Launch client(s)
    my $i;
    for ($i = 0; $i < $num_clients; $i++)
    {
	client ($client, $i, $tempfile);
    }

    # Wait for all but one (the server) to finish
    my $fail_reason = "";
    while ($#pids > 0 && (my $kid = wait) != -1)
    {
	if ($?)
	{
	    $fail = 1;
	    my $rv = $? >> 8;
	    if ($rv == 2)    
	      { $fail_reason = $fail_reason . " (server failed)"; }
	    elsif ($rv == 3) 
	      { $fail_reason = $fail_reason . " (client failed)"; }
	    elsif ($rv == 4) 
	      { $fail_reason = $fail_reason . " (bad client output)"; }
	    elsif ($rv == 5) 
	      { $fail_reason = $fail_reason . " (client timeout)"; }
	}	    

	print STDERR "KID $kid DONE (status $?)\n" if $debug;

	if (-f 'core')
	{
	    print STDERR "KID $kid dumped core\n" if $debug;
	    system ("rm -f core");
	    $fail = 1;
	}

	for ($i = 0; $i <= $#pids; $i++)
	{
	    if ($pids[$i] == $kid)
	    {
		splice (@pids, $i, 1);
		last;
	    }
	}
    }

    kill_processes();

    if ($fail)
    {
	print "FAIL$fail_reason\n";
	$failed = 1;
    }
    else
    {
	print "PASS\n";
    }

    return $fail;
}




####################

sub server
{
    my $server = $_[0];
    my $tempfile = ".server." . $_[1] . ".out";
    my $pid;

    print STDERR "start $tempfile server\n" if $debug;

    if ($pid = fork)         # Parent
    {
	print STDERR "server pid $pid\n" if $debug;
	push (@pids, $pid);
    }
    elsif (defined $pid)     # Child
    {

	setpgrp (0, $$);
	system ("$bin_dir/$server $port > $tempfile 2>&1");
	exit 2;   # This shouldn't happen
    }
    else                     # Error
    { 
	die ("Fork error: $!\n"); 
    }
}


sub client
{
    my $client = $_[0];
    my $num = $_[1];
    my $tempfile = ".client.$num." . $_[2] . ".out";
    my $difffile = ".client.$num." . $_[2] . ".diff";
    my $pid;

    print STDERR "start $tempfile client $num\n" if $debug;

    if ($pid = fork)         # Parent
    {
	print STDERR "client pid $pid\n" if $debug;
	push (@pids, $pid);
    }
    elsif (defined $pid)     # Child
    {
	$SIG{'ALRM'} = sub { exit 5; };

	setpgrp (0, $$);

	alarm 15;
	system ("$bin_dir/$client localhost $port < $testfile > $tempfile 2>&1");
	exit 3 if $? != 0;
	alarm 0;

	system ("diff -a $tempfile $testfile > $difffile 2>&1");
	exit 4 if $? != 0;

	exit 0;
    }
    else                     # Error
    { 
	die ("Fork error: $!\n"); 
    }
}


sub on_sigint
{
    print STDERR "on_sigint()\n" if $debug;

    kill_processes ();
    exit 0;
}


sub kill_processes
{
    print STDERR "kill_processes()\n" if $debug;

    # Kill all children
    foreach $pid (@pids)
    {
	kill (-9, $pid);
	waitpid ($pid, 0);
    }
    @pids = ();
}

#!/usr/bin/perl 

my $templspec = $ARGV[0];
shift(@ARGV);
my $cmdline = "@ARGV";
my $template = $cmdline;

%subst = ();

my @templspeclist = split(",", $templspec);
foreach my $tspec (@templspeclist)
{
    if( $tspec=~/(.*)=(.*)/ ) {
    $subst{$1}=$2;
    }
}



open(TEMPLATEFILE, "$template") || die "Error opening file $template.\n";
while(<TEMPLATEFILE>)
{
    $line=$_;
    foreach my $key (keys %subst)
    {
    $line=~s/(\s+)$key(\s*)/$1$subst{$key}$2/g;
    }
    print $line;
}

close TEMPLATEFILE;

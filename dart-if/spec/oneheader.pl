#!/usr/bin/perl

use File::Basename;
use POSIX qw( strftime );

$dt   = strftime("%Y-%m-%d %H:%M:%S", localtime());
$user = `whoami`; chomp $user;
$host = `hostname`; chomp $host;
$dir  = dirname($ARGV[0]);

print <<EOF;
/* WARNING  --  WARNING  --  WARNING 
 *
 * This is a generated file - do not edit! 
 * 
 * Generated at $dt 
 * By user $user on host $host
 * 
 * Specification source directory: $dir
 */

EOF
;
 
while(<>) {
    if( /#include "(dart_.*)"/ ) {
	$hfile = $1;
#	print "/* included from \"$hfile\" */\n";
	readhdr($hfile);
    } else {
	print "$_";
    }
}

sub readhdr() 
{
    $on=0;
    $fname = shift;
    unless( -e $fname )  {
	$fname="$dir/$fname";
    }

    open(FILE, "$fname") || die "Error opening file: '$fname'\n";
    
    while(<FILE>) {
	if(/DART_INTERFACE_ON/) {
	    $on=1; next;
	}
	if(/DART_INTERFACE_OFF/) {
	    $on=0; next;
	}
	
	if($on) {
	    print "$_";
	}
    }
    
    close(FILE);
}

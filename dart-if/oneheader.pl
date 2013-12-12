#!/usr/bin/perl

print <<EOF;
/* 
 * WARNING: This is a generated file - do not edit! 
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

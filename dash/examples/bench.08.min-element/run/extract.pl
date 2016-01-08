#!/usr/bin/perl

%units = ();
%sizes = ();
%values = ();

while(<>) {
    if( /NUNIT:\s+(\d+).*NELEM:\s+(\d+).*REPEAT:\s+(\d+).*TIME \[msec\]:\s+([\d\.]+)/ ) {
	$nunit = $1;
	$nelem = $2;
	$repeat = $3;
	$time = $4;

	$units{$nunit}=$nunit;
	$sizes{$nelem}=$nelem;
	
	$values{"$nunit#$nelem"} = $time/$repeat;
    }
}

print "size";
foreach my $size (sort {$a<=>$b} keys %sizes) {
    print ", $size";
}
print "\n";

foreach my $unit (sort {$a<=>$b} keys %units) {
    print "$unit";
    foreach my $size (sort keys %sizes) {
	my $val = $values{"$unit#$size"};
	print ", $val";
    }
    print "\n";
}
#foreach $unit 
#	print "** $nunit ** $nelem ** $repeat ** $time\n";


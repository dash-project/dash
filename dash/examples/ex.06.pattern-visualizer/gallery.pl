#!/usr/bin/perl

my $prog_head= <<'END_PROGRAM';
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <cstddef>

#include <libdash.h>

using std::cout;
using std::endl;

using namespace dash;

int main(int argc, char* argv[])
{
    dash::init(&argc, &argv);

    if(dash::myid()==0) {
END_PROGRAM

my $prog_tail= <<'END_PROGRAM';
    }
    dash::finalize();
}
END_PROGRAM

my $nunits=0;
my $teamspec0=0;
my $teamspec1=0;
my $pattern="";
my $counter=0;

foreach $teamspec0 (1,2,3,4)
{
    foreach $teamspec1 (1,2,3,4)
    {
	foreach $pattern (
	    "Pattern<2>(20,15, TILE(1), TILE(5))",
	    "Pattern<2,COL_MAJOR>(20,15, TILE(1), TILE(5))",
	    "Pattern<2>(20,15, TILE(5), TILE(5))",
	    )
	{
	    foreach $pattype (
		"", "Tile", "ShiftTile" )
	    {
		$nunits=$teamspec0*$teamspec1;
		print "$nunits = $teamspec0 x $teamspec1 - $pattern\n";
		
		$counter++;
		print_program("$pattype$pattern", $nunits, $teamspec0, $teamspec1, $counter);
	    }
	}
    }
}


#while(<>) {
#    if(/^nunits=(\d+)/) {
#	$nunits=$1;
#   }
#    if(/^teamspec=(\d+)x(\d+)/) {
#	$teamspec0=$1;
#	$teamspec1=$2;
#   }
#    if(/^pattern=(.*)$/) {
#	$pattern=$1;
#	$counter++;
#	print_program($pattern, $nunits, $teamspec0, $teamspec1, $counter);
#    }
#    if(/---/) {
#	$nunits=0;
#	$teamspec0=0;
#	$teamspec1=0;
#	$pattern="";
#   }
#}

sub print_program()
{
    my $pattern   = shift();
    my $nunits    = shift();
    my $teamspec0 = shift();
    my $teamspec1 = shift();
    my $cnt       = shift();

    $pattern =~ s/^(.*)\)$/$1,TeamSpec<2>($teamspec0,$teamspec1))/;

    print "pattern_$cnt: $pattern\n";

    open(FH, ">", "pattern_$counter.cpp");
    print FH "$prog_head";

    print FH "auto pat = $pattern;\n";
    print FH "PatternVisualizer<decltype(pat)> pv(pat);\n";
    print FH "std::array<int, pat.ndim()> coords = {};\n";
    print FH "pv.set_title(\"$pattern\");\n";
    print FH "pv.draw_pattern(cout, coords, 1, 0);\n";

    print FH "$prog_tail";
    close FH;

    $dash_root = "/home/fuerling/code/dash/";
    $dart_inc  = "$dash_root/dart-if/v3.2/include";
    $dash_inc  = "$dash_root/dash/include";
    $libdart   = "$dash_root/dart-impl/mpi/src/libdart.a";
    $libdash   = "$dash_root/dash/src/libdash.a";

    system "mpicxx -c -O3 -std=c++11 -I$dart_inc -I$dash_inc pattern_$cnt.cpp";
    system "mpicxx -O3 -std=c++11 -o pattern_$cnt pattern_$cnt.o $libdash $libdart";
    system "mpirun -n $nunits ./pattern_$cnt > ~/Dropbox/pattern_$cnt.svg";
}



#!/usr/bin/perl

use strict;
use File::Basename;

my $rdir    = "RELEASE";
my $version = "0.3.0";
my $base    = "$rdir/dash-$version";

if( -e "$base" ) {
    print "Directory $base exists, exiting.\n";
    exit;
}

my @files;

@files = ("LICENSE",
	  "README.md",
	  "CHANGELOG.md",
	  "CMakeLists.txt",
	  "CMakeExt/*.cmake",
          "CMakeExt/Code/*.c",
	  "build.sh",
	  "build.dev.sh",
	  "build.minimal.sh",
	  "config/*.cmake",
	  #
	  # DART interface specification
	  #
	  "dart-if/include/dash/dart/if/*.h",
	  "dart-if/CMakeLists.txt",
	  #
	  # DART base implementation
	  #
	  "dart-impl/CMakeLists.txt",
	  "dart-impl/base/include/dash/dart/base/*.h",
	  "dart-impl/base/include/dash/dart/base/internal/*.h",
	  "dart-impl/base/src/*.c",
	  "dart-impl/base/src/internal/*.c",
	  "dart-impl/base/CMakeLists.txt",
	  #
	  # DART MPI implementation
	  #
	  "dart-impl/mpi/include/dash/dart/mpi/*.h",
	  "dart-impl/mpi/src/*.c",
	  "dart-impl/mpi/src/Makefile",
	  "dart-impl/mpi/make.defs",
	  "dart-impl/mpi/CMakeLists.txt",
	  "dart-impl/mpi/*.in",
	  #
	  # DASH source and header files
	  #
	  "dash/include/*.h",
	  "dash/include/dash/*.h",
	  "dash/include/dash/algorithm/*.h",
	  "dash/include/dash/algorithm/internal/*.h",
	  "dash/include/dash/allocator/*.h",
	  "dash/include/dash/allocator/internal/*.h",
	  "dash/include/dash/atomic/*.h",
	  "dash/include/dash/bindings/*.h",
	  "dash/include/dash/coarray/*.h",
	  "dash/include/dash/exception/*.h",
	  "dash/include/dash/halo/*.h",
	  "dash/include/dash/halo/iterator/*.h",
	  "dash/include/dash/internal/*.h",
	  "dash/include/dash/io/*.h",
	  "dash/include/dash/io/hdf5/*.h",
	  "dash/include/dash/io/hdf5/internal/*.h",
	  "dash/include/dash/iterator/*.h",
	  "dash/include/dash/iterator/internal/*.h",
	  "dash/include/dash/list/*.h",
	  "dash/include/dash/list/internal/*.h",
	  "dash/include/dash/map/*.h",
	  "dash/include/dash/matrix/*.h",
	  "dash/include/dash/matrix/internal/*.h",
	  "dash/include/dash/memory/*.h",
	  "dash/include/dash/memory/internal/*.h",
	  "dash/include/dash/meta/*.h",
	  "dash/include/dash/pattern/*.h",
	  "dash/include/dash/pattern/internal/*.h",
	  "dash/include/dash/tools/*.h",
	  "dash/include/dash/util/*.h",
	  "dash/include/dash/util/*.h.in",
	  "dash/include/dash/util/internal/*.h",
	  "dash/include/dash/view/*.h",
	  "dash/src/*.cc",
	  "dash/src/algorithm/*.cc",
	  "dash/src/exception/*.cc",
	  "dash/src/io/*.cc",
	  "dash/src/util/*.cc",
	  "dash/src/Makefile",
	  "dash/make.defs",
	  "dash/CMakeLists.txt",
          "dash/dash-config.cmake.in",
	  #
	  # DASH examples
	  #
	  "dash/examples/Makefile_cpp",
	  "dash/examples/*.h",
	  "dash/examples/ex*/*.h",
	  "dash/examples/ex*/*.cpp",
	  "dash/examples/ex*/*.cc",
	  "dash/examples/ex*/Makefile",
	  "dash/examples/bench*/*.cpp",
	  "dash/examples/bench*/*.cc",
	  "dash/examples/bench*/*.h",
	  "dash/examples/bench*/Makefile",
	  "dash/examples/Makefile",
#	  "dash/examples/CMakeLists.txt",
	  #
	  # DASH tests
	  #
	  "dash/test/Makefile",
#	  "dash/test/CMakeLists.txt",
	  "dash/test/*.h",
	  "dash/test/*.cc",
	  "dash/test/*.cpp",
	  #
	  # DASH scripts
	  #
	  "dash/scripts/*.sh",
          "dash/scripts/dashcc/*.in",
          #
	  # Documentation
	  #
	  "doc/config/*.dox",
	  "doc/config/*.in"
        );


foreach my $path (@files)
{
    foreach my $file (glob($path))
    {
	my $dirname  = dirname("$base/$file");

	system("mkdir -p $dirname");

	print "copying '$file'\n";

	# prepend license file for h,c,cc,cpp files
	if( $file =~ /\.(c|h|cc|cpp)$/ && -e "LICENSE" ) {
	    system("cat ./LICENSE $file > $base/$file");
	} else {
	    system("cp $file $dirname");
	}
    }
}

# use a fixed date (yesterday) and time to make sure the md5sum
# only depends on the content and not on the time-stamps
my $date = `date +%F --date yesterday`; chomp($date);
$date = $date." 10:02:33";

print "\nCreating tarball and cleaning up... ";
system("cd $rdir; tar --mtime='$date' -cf dash-$version.tar dash-$version/");
system("cd $rdir; touch -d '$date' dash-$version.tar");
system("cd $rdir; gzip -f dash-$version.tar");
system("rm -rf $base");
print "DONE!\n";

if( -e "$base.tar.gz" ) {
    my $md5sum = `md5sum $base.tar.gz | cut -d ' ' -f 1`; chomp $md5sum;
    my $fsize  = `ls -l $base.tar.gz | cut -d ' ' -f 5`; chomp $fsize;

    print "\n";
    print "DASH $version release built!\n";
    print "------------------------------------------------\n";
    print "archive   : $base.tar.gz\n";
    print "md5sum    : $md5sum\n";
    print "file size : $fsize bytes\n";
    print "------------------------------------------------\n";
} else {
    print "Something went wrong, check directory '$rdir'\n";
}


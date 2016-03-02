#!/usr/bin/perl

use File::Basename;
#use File::Slurp;

$rdir    = "RELEASE";
$version = "0.2.0";
$base    = "$rdir/dash-$version";

if( -e "$base" ) {
    print "Directory $base exists, exiting.\n";
    exit;
}

#$license = read_file("LICENSE");

@files = ("LICENSE",
	  "README.md",
	  "CHANGELOG.md",
	  "CMakeLists.txt",
	  "CMakeExt/*.cmake",
	  "build.sh",
	  "build-dev.sh",
	  "config/*.cmake",
	  #
	  # DART interface specification
	  #
	  "dart-if/v3.2/include/dash/dart/if/*.h",
	  "dart-if/CMakeLists.txt",
	  #
	  # DART base implementation
	  #
	  "dart-impl/base/include/dash/dart/base/*.h",
	  "dart-impl/base/src/*.c",
	  "dart-impl/base/CMakeLists.txt",
	  #
	  # DART MPI implementation
	  #
	  "dart-impl/mpi/include/dash/dart/mpi/*.h",
	  "dart-impl/mpi/src/*.c",
	  "dart-impl/mpi/src/Makefile",
	  "dart-impl/mpi/make.defs",
	  "dart-impl/mpi/CMakeLists.txt",
	  #
	  # DASH source and header files
	  #
	  "dash/include/*.h",
	  "dash/include/dash/*.h",
	  "dash/include/dash/algorithm/*.h",
	  "dash/include/dash/bindings/*.h",
	  "dash/include/dash/exception/*.h",
	  "dash/include/dash/matrix/*.h",
	  "dash/include/dash/matrix/internal/*.h",
	  "dash/include/dash/tools/*.h",
	  "dash/include/dash/internal/*.h",
	  "dash/include/dash/util/*.h",
	  "dash/include/dash/util/internal/*.h",
	  "dash/src/*.cc",
	  "dash/src/util/*.cc",
	  "dash/src/algorithm/*.cc",
	  "dash/src/Makefile",
	  "dash/make.defs",
	  "dash/CMakeLists.txt",
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
	  "dash/examples/CMakeLists.txt",
	  #
	  # DASH tests
	  #
	  "dash/test/Makefile",
	  "dash/test/CMakeLists.txt",
	  "dash/test/*.h",
	  "dash/test/*.cc",
	  "dash/test/*.cpp",
	  #
	  # DASH scripts
	  #
	  "dash/scripts/*.sh",
	  #
	  # Documentation
	  #
	  "doc/config/*.dox",
	  "doc/config/*.in"
        );


foreach $dir (@files)
{
    foreach $file (glob($dir))
    {
	$dirname  = dirname("$base/$file");

	system("mkdir -p $dirname");

	print "copying '$file'\n";

	# prepend license file for h,c,cc,cpp files
	if( $file =~ /\.(c|h|cc|cpp)$/ ) {
	    system("cat ./LICENSE $file > $base/$file");
	} else {
	    system("cp $file $dirname");
	}
    }
}


print "\nCreating tarball and cleaning up... ";
system("cd $rdir; tar -czf dash-$version.tar.gz dash-$version/");
system("rm -rf $base");
print "DONE!\n";

if( -e "$base.tar.gz" ) {
    print "DASH is here: $base.tar.gz\n";
} else {
    print "Something went wrong, check directory '$rdir'\n";
}


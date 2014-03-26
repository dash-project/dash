#README for testlayout
Tests are grouped into the following 8 categories:

01: init 	(Initialization/Hello World)
02: groups	(Groups)
03: teams	(Teams)
04: gptr	(Global Pointers)
05: mem		(Memory Management) 
06: collectives	(Collective Communication)
07: onesided 	(Onesided Communication)
08: sync 	(Synchronization/Locks)
09: p2p  	(Point to Point Communication)

When writing a test, add a new directory named
test.xx.<Categoryname>_<Testname>

If it is a general functionality test, Testname can be omitted.

The directory has to have 
-a Makefile containing only this line:"include ../Makefile_c"
-main.c

The name of the test should be informative.
Add a Comment describing your test before the main methode!

When calling make, the exectuable <Categoryname>_<Testname> is created. Run it using 
dartrun -n <n> <executable> <args>

For implementation specific tests, move these to dart-impl/<specific_implementation>/test

A more elaborate testing framework for distributed testing is under development.


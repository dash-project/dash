
ONEHEADER = ./bin/oneheader.pl
CC        = clang
CPPC      = g++
#CFLAGS    = -std=c99 

LIBDART = dart-dummy/libdart-dummy.a

DART_FILES = dart_init.h dart_types.h dart_groups.h dart_teams.h    \
	dart_gptr.h dart_memory.h dart_collective.h dart_onesided.h

all : dart_full.h $(LIBDART) test/test

dart_full.h : dart.h $(DART_FILES)
	$(ONEHEADER) ./dart.h > dart_full.h

dart-dummy/libdart-dummy.a : 
	make -C dart-dummy/

test/test : 
	make -C test/

clean :
	rm -f *~
	rm -f *.o 
	rm -f dart_full.h
	make -C dart-dummy/ clean
	make -C test/ clean




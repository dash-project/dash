
include make.defs

ONEHEADER = $(DART_IF)/../oneheader.pl

LIBDART = dart-shmem/libdart-shmem.a

# components of the dart interface spec
DART_COMP = dart.h dart_types.h dart_initialization.h \
	dart_team_group.h dart_globmem.h \
	dart_communication.h dart_synchronization.h

DART_FILES = $(addprefix $(DART_IF)/, $(DART_COMP))

all : $(LIBDART) test

test : 	$(LIBDART)
	make -C test

dart_full.h : $(DART_IF)/dart.h $(DART_FILES)
	$(ONEHEADER) $(DART_IF)/dart.h > dart_full.h

dart-shmem/libdart-shmem.a : 
	make -C dart-shmem/

clean :
	rm -f *~
	rm -f *.o 
	rm -f dart_full.h
	make -C dart-shmem/ clean





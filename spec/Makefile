
include make.defs

ONEHEADER = ./bin/oneheader.pl

LIBDART = dart-dummy/libdart-dummy.a

DART_FILES = dart_init.h dart_types.h dart_groups.h dart_teams.h    \
	dart_gptr.h dart_memory.h dart_collective.h dart_onesided.h

all : dart_full.h dart_full.html $(LIBDART) test/test

publish : dart_full.html
	cp highlight.css dart_full.html $(HOME)/Dropbox/Public

dart_full.html : dart_full.h
	highlight --style moria --syntax c -i dart_full.h  -o dart_full.html

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
	rm -f dart_full.html
	rm -f highlight.css
	make -C dart-dummy/ clean
	make -C test/ clean




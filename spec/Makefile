

include make.defs

ONEHEADER = ./bin/oneheader.pl

LIBDART = dart-dummy/libdart-dummy.a

DART_FILES = dart_types.h dart_initialization.h dart_team_group.h \
	dart_globmem.h dart_communication.h dart_synchronization.h

all : dart_full.h dart_full.html $(LIBDART) test/test

publish : dart_full.html
	cp highlight.css dart_full.html $(HOME)/Dropbox/Public

deploy : 
	cp $(DART_FILES) dart.h ../dart-new

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




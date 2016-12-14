
ONEHEADER = ../../../../../spec/oneheader.pl

DART_SPEC = dart_spec_v4.0

DART_FILES = 			\
	dart_types.h 		\
	dart_config.h 		\
	dart_initialization.h 	\
	dart_team_group.h 	\
	dart_globmem.h 		\
	dart_collective.h	\
	dart_onesided.h 	\
	dart_twosided.h 	\
	dart_synchronization.h


all : html

publish : html
	cp highlight.css $(DART_SPEC).html $(DART_SPEC).h \
	$(HOME)/Dropbox/Public

html : $(DART_SPEC).html

$(DART_SPEC).html : $(DART_SPEC).h
	highlight --style moria --syntax c -i $(DART_SPEC).h  -o $(DART_SPEC).html


$(DART_SPEC).h : dart.h $(DART_FILES)
	$(ONEHEADER) ./dart.h > $(DART_SPEC).h

doxygen : dart.h
	doxygen -g
	doxygen Doxyfile


clean :
	rm -f *~
	rm -f *.o
	rm -f $(DART_SPEC).h
	rm -f $(DART_SPEC).html
	rm -f highlight.css
	rm -rf latex/
	rm -rf html/
	rm -f Doxyfile*




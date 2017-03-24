
ONEHEADER = ./spec/oneheader.pl

DART_SPEC = dart_spec

DART_H    = $(addprefix include/dash/dart/if/, dart.h)

DART_FILES = dart_types.h dart_initialization.h dart_team_group.h \
	dart_globmem.h dart_communication.h dart_synchronization.h

all : html

publish : html
	cp highlight.css $(DART_SPEC).html $(HOME)/Dropbox/Public

html : $(DART_SPEC).html

$(DART_SPEC).html : $(DART_SPEC).h
	highlight --style moria --syntax c -i $(DART_SPEC).h  -o $(DART_SPEC).html


$(DART_SPEC).h : $(DART_H) $(addprefix include/dash/dart/if/, $(DART_FILES))
	$(ONEHEADER) $(DART_H) > $(DART_SPEC).h

clean :
	rm -f *~
	rm -f *.o
	rm -f $(DART_SPEC).h
	rm -f $(DART_SPEC).html
	rm -f highlight.css





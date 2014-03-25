
DART_VER   = v2.2
DART_IF    = ../../dart-if/$(DART_VER)
ONEHEADER  = ./oneheader.pl

DART_SPEC  = dart_spec_$(DART_VER)

DART_COMP = dart_types.h dart_initialization.h dart_team_group.h \
	dart_globmem.h dart_communication.h dart_synchronization.h

DART_FILES = $(addprefix $(DART_IF)/, $(DART_COMP))


all : spec 

spec : $(DART_SPEC).h

html : $(DART_SPEC).html

$(DART_SPEC).h : $(DART_IF)/dart.h $(DART_IF)/$(DART_FILES)
	$(ONEHEADER) $(DART_IF)/dart.h > $(DART_SPEC).h

$(DART_SPEC).html: $(DART_SPEC).h
	highlight --style moria --syntax c -i $(DART_SPEC).h  \
		 -o $(DART_SPEC).html

publish : $(DART_SPEC).html
	cp highlight.css $(DART_SPEC).html $(HOME)/Dropbox/Public

clean :
	rm -f *~
	rm -f *.o 
	rm -f $(DART_SPEC).h
	rm -f $(DART_SPEC).html
	rm -f highlight.css

DASH_ROOT=..
include ../make.defs

CXXFLAGS+=-I$(DART_INC)

#CXXFLAGS+=-DDASH_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDASH = libdash.a

FILES = Init Team Pattern

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDASH) 


$(LIBDASH) : $(OBJS)
	ar rcs $(LIBDASH) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDASH)

printenv : 
	 @echo "DART_IMPL   = $(DART_IMPL)"
	 @echo "DASH_ROOT   = $(DASH_ROOT)"
	 @echo "LIBDART_DIR = $(LIBDART_DIR)"
	 @echo "LIBDART     = $(LIBDART)"
	 @echo "DART_INC    = $(DART_INC)"


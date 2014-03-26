include ../make.defs

CFLAGS+=-I$(DART_IF) 

#CFLAGS+=-DDART_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDART  = libdart.a

LIBDASH = libdash.a

FILES = Team Init

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDASH) 

$(LIBDASH) : $(OBJS)
	ar rcs $(LIBDASH) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDASH)



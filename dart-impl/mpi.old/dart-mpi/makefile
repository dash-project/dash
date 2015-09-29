include ../make.defs


#CFLAGS+=-I. -I../$(DART_IF) 
CFLAGS+=-I.

#CFLAGS+=-DDART_DEBUG

LIBDART  = libdart.a
LIBDART_ADAPT = adapt/libdart-adapt.a

FILES = \
	dart_core_communication \
	dart_core_globmem \
	dart_core_initialization \
	dart_core_synchronization \
	dart_core_team_group

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART_ADAPT) $(LIBDART)

$(LIBDART) : $(OBJS)  
	ar rcs $(LIBDART) $(OBJS)

$(LIBDART_ADAPT) :
	make -C adapt


../dart_all.h : 
	make -C ../ dart_all.h

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)
	make -C adapt/ cleanall

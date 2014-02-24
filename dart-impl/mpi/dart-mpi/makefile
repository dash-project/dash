include ../make.defs


CFLAGS+=-I.. -I../$(DART_IF) 

#CFLAGS+=-DDART_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDART  = libdart.a
LIBDART_ADAPT = libdart-adapt.a

#FILES_ADAPT = \
#	adapt/dart_adapt_communication \
#	adapt/dart_adapt_globmem \
#	adapt/dart_adapt_initialization \
#	adapt/dart_adapt_mem \
#	adapt/dart_adapt_synchronization \
#	adapt/dart_adapt_team_group \
#	adapt/dart_adapt_teamnode \
#	adapt/dart_adapt_translation
FILES = \
	dart_core_communication \
	dart_core_globmem \
	dart_core_initialization \
	dart_core_synchronization \
	dart_core_team_group

OBJS = $(addsuffix .o, $(FILES))
#OBJS_ADAPT = $(addsuffix .o, $(FILES_ADAPT))

#$(FILES_ADAPT).o:$(FILES_ADAPT).c
#	cc dasd dasdas

all : $(LIBDART_ADAPT) $(LIBDART)

$(LIBDART) : $(OBJS)  
	ar rcs $(LIBDART) $(OBJS)

$(LIBDART_ADAPT) :
	make -C adapt
#	ar rcs $(LIBDART_ADAPT) $(OBJS_ADAPT)


../dart_all.h : 
	make -C ../ dart_all.h

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART) $(LIBDART_ADAPT)
	#make -C core/ clean
	#make -C adapt/ clean

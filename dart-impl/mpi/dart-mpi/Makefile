include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF) 

#CFLAGS+=-DDART_DEBUG

LIBDART  = libdart.a

FILES = dart_adapt_communication    	\
	dart_adapt_globmem		\
	dart_adapt_initialization	\
	dart_adapt_mem			\
	dart_adapt_synchronization	\
	dart_adapt_team_group		\
	dart_adapt_team_private		\
	dart_adapt_translation

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART) 

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)

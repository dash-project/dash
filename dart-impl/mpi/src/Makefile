include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF)/include -I../include -I../../base/include
#CFLAGS+=-DDART_ENABLE_SHARED_MEMORY
#CFLAGS+=-DDART_DEBUG
#OPT_FLAGS=-O3

LIBDART  = libdart.a

FILES = dart_communication    	\
	dart_globmem		\
	dart_initialization	\
	dart_mem			\
	dart_synchronization	\
	dart_team_group		\
	dart_team_private		\
	dart_translation

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART) 

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)

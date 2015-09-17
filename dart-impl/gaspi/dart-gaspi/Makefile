include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF)

#CFLAGS+=-DDART_DEBUG

LIBDART  = libdart.a

FILES = gaspi_utils \
	dart_initialization \
	dart_team_group \
	dart_group_impl \
	dart_communication \
	dart_team_private \
	dart_seg_stack \
	dart_translation \
	dart_globmem

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART)

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

clean :
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)

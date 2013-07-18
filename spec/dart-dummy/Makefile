
include ../make.defs

CFLAGS += -I..

LIBDART  = libdart.a

FILES = dart_groups_impl dart_onesided_impl dart_init_impl \
	dart_collective_impl dart_teams_impl dart_memory_impl \
	dart_locks_impl


OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART)

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

../dart_all.h : 
	make -C ../ dart_all.h

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)


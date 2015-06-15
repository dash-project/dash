#
# In-place makefile for use side-by-side with the 
# CMake build system
#

include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF)/include -I../include 
#OPT_FLAGS=-O3

#CFLAGS+=-DDART_DEBUG
CFLAGS+=-D_SVID_SOURCE
CFLAGS+=-DUSE_HELPER_THREAD=1 -DUSE_EVENTFD

LIBDART  = libdart.a

FILES = dart_init_impl dart_init_shmem 	    	\
	dart_groups_impl dart_teams_impl	\
	dart_collective_impl			\
	shmem_barriers_sysv 			\
	shmem_mm_sysv				\
	shmem_p2p_sysv				\
	dart_memarea				\
	dart_mempool				\
	dart_membucket				\
	dart_malloc				\
	dart_onesided				\
	dart_helper_thread

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART) dartrun

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

dartrun : dartrun.c $(LIBDART)
	$(CC) $(CFLAGS) -o dartrun dartrun.c $(LIBDART) -lpthread

../dart_all.h : 
	make -C ../ dart_all.h

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)
	rm -f ./dartrun

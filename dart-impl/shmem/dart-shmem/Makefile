include ../make.defs


CFLAGS+=-I.. -I../$(DART_IF) 

#CFLAGS+=-DDART_DEBUG
CFLAGS+=-D_SVID_SOURCE

LIBDART  = libdart.a

FILES = dart_init_impl dart_init_shmem 	    	\
	dart_groups_impl dart_teams_impl	\
	dart_collective_impl			\
	sysv/shmem_barriers_sysv 		\
	sysv/shmem_mm_sysv			\
	sysv/shmem_p2p_sysv			\
	dart_mempool				\
	dart_memarea				\
	dart_malloc				\
	dart_onesided


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
	make -C sysv/ clean

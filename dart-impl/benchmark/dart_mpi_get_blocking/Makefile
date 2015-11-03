BENCH_ROOT  = $(PWD)/../
DART_ROOT  = /home/cherold/procs/dash

CFLAGS    = -Wall --std=gnu99 -c -ggdb -O3
LIB_DART  = -L$(DART_ROOT)/lib -ldart-mpi
LIB       =  $(LIB_DART)

INCLUDE   = -I$(BENCH_ROOT) -I$(DART_ROOT)/include

CC = mpicc

PROG = main

$(PROG):main.o
	$(CC) -o $@ main.o $(LIB)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) $<

clean:
	rm -rf *~ \#_* *.o
	rm main

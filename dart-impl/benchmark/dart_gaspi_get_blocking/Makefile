BENCH_ROOT  = $(PWD)/../
DART_ROOT  = /home/cherold/procs/dash

CFLAGS    = -Wall --std=gnu99 -c -ggdb -pg -O0
LIB_DART  = -L$(DART_ROOT)/lib -ldart-gaspi
LIB_GASPI = -L$(GPI2_ROOT)/lib -libverbs -lpthread -lGPI2 -Wl,-rpath=$(GPI2_ROOT)/lib:
LIB       =  $(LIB_DART) $(LIB_GASPI)

INCLUDE   = -I$(BENCH_ROOT) -I$(DART_ROOT)/include -I$(GPI2_ROOT)/include

CC = gcc

PROG = main

$(PROG):main.o
	$(CC) -pg -o $@ main.o $(LIB)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) $<

clean:
	rm -rf *~ \#_* *.o
	rm main

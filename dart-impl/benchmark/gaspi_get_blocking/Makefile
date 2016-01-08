BENCH_ROOT  = $(PWD)/../

CFLAGS    = -Wall --std=gnu99 -c -O3
LIB_GASPI = -L$(GPI2_ROOT)/lib -libverbs -lpthread -lGPI2 -Wl,-rpath=$(GPI2_ROOT)/lib:
LIB       =  $(LIB_GASPI)

INCLUDE   = -I$(BENCH_ROOT) -I$(GPI2_ROOT)/include

CC = gcc

PROG = main

$(PROG):main.o
	$(CC) -o $@ main.o $(LIB)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) $<

clean:
	rm -rf *~ \#_* *.o
	rm main

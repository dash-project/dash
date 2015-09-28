DART_ROOT = $(PWD)/../..

CFLAGS    = -std=gnu99 -Wall -c -g3 -O0
LIB_DART  = -L$(DART_ROOT)/dart-gaspi/ -ldart
INCLUDE   = -I$(DART_ROOT)/dart-gaspi -I$(DART_ROOT)/../../dart-if/v3.1 -I$(GPI2_ROOT)/include

PROG = main

$(PROG):main.o
	$(CC) -o $@ main.o $(LIB_DART)

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) $<

clean:
	rm -rf *~ \#_* *.o
	rm main

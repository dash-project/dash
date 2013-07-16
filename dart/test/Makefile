
CC   = gcc -std=c99
CPPC = g++ -std=c++0x
CFLAGS = -I..

LIBDART = ../dart-dummy/libdart.a

all : test test_cpp

test : test.c $(LIBDART)
	$(CC) $(CFLAGS) -o test test.c $(LIBDART) 

test_cpp : test.c $(LIBDART)
	$(CPPC) $(CFLAGS) -o test_cpp test.c $(LIBDART) 

$(LIBDART) : 
	make -C ../dart-dummy/

clean : 
	rm -f *~
	rm -f *.o
	rm -f test test_cpp


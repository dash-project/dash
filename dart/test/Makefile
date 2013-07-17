
include ../make.defs

CFLAGS += -I.. -I../dart-dummy

LIBDART = ../dart-dummy/libdart.a

all : test test_cpp distarray

test : test.c $(LIBDART)
	$(CC) $(CFLAGS) -o test test.c $(LIBDART) 

test_cpp : test.c $(LIBDART)
	$(CXX) $(CFLAGS) -o test_cpp test.c $(LIBDART) 

distarray : distarray.c $(LIBDART)
	$(CC) $(CFLAGS) -o distarray distarray.c $(LIBDART) 



$(LIBDART) : 
	make -C ../dart-dummy/

clean : 
	rm -f *~
	rm -f *.o
	rm -f test test_cpp distarray


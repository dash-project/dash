all: test_basic test_onesided test_collective_comm
OBJS=test_basic.o test_onesided.o test_collective_comm.o
CC=cc
INCLIB=../dart

LIBS=../dart-mpi/tlib.a
CFLAGS=-g -I. -I$(INCLIB)

.c.o:
	$(CC) $(CFLAGS) -c $<
test_basic: test_basic.o $(LIBS)
	$(CC) $(CFLAGS) -o test_basic test_basic.o $(LIBS)
test_onesided: test_onesided.o $(LIBS)
	$(CC) $(CFLAGS) -o test_onesided test_onesided.o $(LIBS)
test_collective_comm: test_collective_comm.o $(LIBS)
	$(CC) $(CFLAGS) -o test_collective_comm test_collective_comm.o $(LIBS)
$(OBJS): ./dart_app_privates.h $(INCLIB)/dart.h

cleanall:
	-rm -f *.o test_basic test_onesided test_collective_comm


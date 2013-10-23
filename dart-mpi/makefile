TLIB=tlib.a
OBJS= dart_team_group.o dart_teamnode.o dart_translation.o dart_mem.o dart_initialization.o dart_communication.o dart_globmem.o mpi_team_private.o
CC=cc
DARTPATH=../dart
LOCALPATH=.
CFLAGS=-g -I$(LOCALPATH) -I$(DARTPATH)


.c.o:
	$(CC) $(CFLAGS) -c $<

$(TLIB): $(OBJS)
	ar cr $(TLIB) $(OBJS)

$(OBJS): ./mpi_team_private.h $(LOCALPATH)/dart_mem.h $(LOCALPATH)/dart_translation.h $(LOCALPATH)/dart_teamnode.h $(DARTPATH)/dart_types.h $(DARTPATH)/dart_team_group.h $(DARTPATH)/dart_initialization.h $(DARTPATH)/dart_communication.h $(DARTPATH)/dart_globmem.h

cleanall:
	-rm -f *.o *.a

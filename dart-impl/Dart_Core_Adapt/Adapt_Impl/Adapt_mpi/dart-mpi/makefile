TLIB=tlib.a
OBJS= dart_adapt_translation.o mpi_adapt_team_private.o dart_adapt_team_group.o dart_adapt_teamnode.o dart_adapt_mem.o dart_adapt_initialization.o dart_adapt_globmem.o dart_adapt_communication.o dart_adapt_synchronization.o 
CC=cc
DARTPATH=../dart
LOCALPATH=.
CFLAGS=-h msglevel_4 -g -I$(LOCALPATH) -I$(DARTPATH)


.c.o:
	$(CC) $(CFLAGS) -c $<

$(TLIB): $(OBJS)
	ar cr $(TLIB) $(OBJS)

$(OBJS): ./dart_deb_log.h ./dart_adapt_translation.h ./mpi_adapt_team_private.h ./dart_adapt_mem.h ./dart_adapt_teamnode.h $(DARTPATH)/dart_types.h $(DARTPATH)/dart_team_group.h $(DARTPATH)/dart_initialization.h $(DARTPATH)/dart_communication.h $(DARTPATH)/dart_globmem.h $(DARTPATH)/dart_synchronization.h

cleanall:
	-rm -f *.o *.a

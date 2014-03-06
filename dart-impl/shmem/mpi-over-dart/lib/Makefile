
DART_ROOT = $(shell pwd)/../..
LIBDART   = $(DART_ROOT)/dart-shmem/libdart.a -lpthread
DART_INC  = $(DART_ROOT)/../../dart-if/v2.1
include $(DART_ROOT)/make.defs

CFLAGS+=-I.. -I../$(DART_IF) 

#CFLAGS+=-DDART_DEBUG
CFLAGS+=-D_SVID_SOURCE
CFLAGS+=-I$(DART_INC) -I../include

LIBDARTMPI  = libdartmpi.a

FILES = mpi_init \
	mpi_sendrecv

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDARTMPI) 

$(LIBDARTMPI) : $(OBJS)
	ar rcs $(LIBDARTMPI) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDARTMPI)

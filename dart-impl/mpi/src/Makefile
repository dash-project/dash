include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF)/include -I../include -I../../base/include
#CFLAGS+=-DDART_MPI_DISABLE_SHARED_WINDOWS
#CFLAGS+=-DDART_DEBUG
#OPT_FLAGS=-O3

LIBDART  = libdart.a

BASE_SRC_PATH=../../base/src
FILES = dart_communication    	\
	dart_globmem		\
	dart_initialization	\
	dart_locality	\
	dart_locality_priv	\
	dart_mem			\
	dart_synchronization	\
	dart_team_group		\
	dart_team_private		\
	dart_translation		\
	$(BASE_SRC_PATH)/hwinfo	\
	$(BASE_SRC_PATH)/locality	\
	$(BASE_SRC_PATH)/logging	\
	$(BASE_SRC_PATH)/internal/domain_locality	\
	$(BASE_SRC_PATH)/internal/unit_locality	\
	$(BASE_SRC_PATH)/internal/host_topology	\
	$(BASE_SRC_PATH)/internal/papi

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDART) 

$(LIBDART) : $(OBJS)
	ar rcs $(LIBDART) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDART)
	rm -f $(BASE_SRC_PATH)/*.o

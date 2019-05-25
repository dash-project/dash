include ../make.defs

CFLAGS+=-I.. -I../$(DART_IF)/include -I../include -I../../base/include -L/usr/lib64 -lnuma
#CFLAGS+=-I $(HOME)/TOOLS/hwloc-1.10.1/BUILD/include
#CFLAGS+=-L$(HOME)/TOOLS/hwloc-1.10.1/BUILD/lib -llibhwloc
#CFLAGS+=-DDART_ENABLE_PAPI
#CFLAGS+=-DDART_ENABLE_HWLOC
CFLAGS+=-DDART_MPI_DISABLE_SHARED_WINDOWS
CFLAGS+=-DDART_MPI_ENABLE_DYNAMIC_WINDOWS
#CFLAGS+=-DDART_DEBUG
#OPT_FLAGS=-O3

LIBDART  = libdart.a

BASE_SRC_PATH=../../base/src

FILES = dart_communication dart_mpi_op dart_config dart_globmem	\
	dart_initialization dart_io_hdf5 dart_locality		\
	dart_locality_priv dart_mem dart_mpi_types dart_segment	\
	dart_synchronization dart_team_group dart_team_private

FILES += $(BASE_SRC_PATH)/array $(BASE_SRC_PATH)/hwinfo		\
	$(BASE_SRC_PATH)/locality $(BASE_SRC_PATH)/logging	\
	$(BASE_SRC_PATH)/string

FILES += $(BASE_SRC_PATH)/internal/domain_locality	\
	$(BASE_SRC_PATH)/internal/unit_locality		\
	$(BASE_SRC_PATH)/internal/host_topology		\
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
	rm -f $(BASE_SRC_PATH)/internal/*.o

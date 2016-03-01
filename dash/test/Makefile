#
# In-place makefile for use side-by-side with the
# CMake build system
#

# Set environments variables pointing to your gtest installation, for
# example:
#
#   export GTEST_INCLUDE_PATH=~/opt/gtest/include
#   export GTEST_LIBRARY_PATH=~/opt/gtest/
#

DASH_ROOT=..
include $(DASH_ROOT)/make.defs

CXXFLAGS+=-I$(DART_INC) -I$(DASH_INC)

ifeq ($(DASH_ENABLE_LOGGING),1)
	CXXFLAGS+=-DDASH_ENABLE_LOGGING
endif

ifeq ($(DASH_ENABLE_TRACE_LOGGING),1)
	CXXFLAGS+=-DDASH_ENABLE_LOGGING
	CXXFLAGS+=-DDASH_ENABLE_TRACE_LOGGING
endif

ifeq ($(DASH_ENABLE_ASSERTIONS),1)
	CXXFLAGS+=-DDASH_ENABLE_ASSERTIONS
endif

ifeq ($(DART_IMPL), mpi)
     CFLAGS+=-DDART_MPI
endif
ifeq ($(DART_IMPL), shmem)
     CFLAGS+=-DDART_SHMEM
endif

LDFLAGS  += -lgtest -L$(GTEST_LIBRARY_PATH) -L$(LIBDART_DIR) -ldart
INCLUDES += -I$(GTEST_INCLUDE_PATH)

DASHTEST = dash-test-mpi

FILES = ArrayTest \
				MatrixTest \
				BlockPatternTest \
				TilePatternTest \
				ShiftTilePatternTest \
				CartesianTest \
				CopyTest \
				FillTest \
				GenerateTest \
				DARTOnesidedTest \
				GlobAsyncRefTest \
				STLAlgorithmTest \
				SUMMATest \
				TeamTest \
				TeamSpecTest \
				TransformTest \
				MakePatternTest \
				MaxElementTest \
				MinElementTest \
				main

OBJS = $(addsuffix .o, $(FILES))

all : $(DASHTEST)

$(DASHTEST) : $(OBJS)
	ar rcs $(DASHTEST) $(OBJS)

%.o 	: %.cc
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) $+

clean :
	rm -f *~
	rm -f *.o
	rm -f $(DASHTEST)

printenv :
	@echo "DART_IMPL   = $(DART_IMPL)"
	@echo "DART_IF     = $(DART_IF)"
	@echo "DASH_ROOT   = $(DASH_ROOT)"
	@echo "LIBDART_DIR = $(LIBDART_DIR)"
	@echo "LIBDART     = $(LIBDART)"
	@echo "DART_INC    = $(DART_INC)"
	@echo "CXX         = $(CXX)"
	@echo "CXXFLAGS    = $(CXXFLAGS)"



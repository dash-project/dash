#
# In-place makefile for use side-by-side with the
# CMake build system
#

DASH_ROOT=..
include $(DASH_ROOT)/make.defs

STATIC_CONFIG_H    = $(DASH_ROOT)/include/dash/util/StaticConfig.h
STATIC_CONFIG_H_IN = $(DASH_ROOT)/include/dash/util/StaticConfig.h.in


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

CXXFLAGS+=-DDASH_ENABLE_DEFAULT_INDEX_TYPE_LONG
CXXFLAGS+=-std=c++11

#CXXFLAGS+=-DDASH_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDASH = libdash.a

FILES = Distribution GlobPtr Init Logging Math Mutex StreamConversion	\
	Team TypeInfo algorithm/SUMMA exception/StackTrace io/IOStream		\
	util/BenchmarkParams util/Config util/Locality						\
	util/LocalityDomain util/LocalityJSONPrinter util/TeamLocality		\
	util/Timer util/TimestampClockPosix util/TimestampCounterPosix		\
	util/TimestampPAPI util/Trace

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDASH)

$(LIBDASH) : $(STATIC_CONFIG_H) $(OBJS)
	ar rcs $(LIBDASH) $(OBJS)


$(STATIC_CONFIG_H) : $(STATIC_CONFIG_H_IN)
	cat $(STATIC_CONFIG_H_IN)                 | \
	sed "s/@CONF_AVAIL_PAPI@/false/"          | \
	sed "s/@CONF_AVAIL_HWLOC@/false/"         | \
	sed "s/@CONF_AVAIL_LIKWID@/false/"        | \
	sed "s/@CONF_AVAIL_NUMA@/false/"          | \
	sed "s/@CONF_AVAIL_PLASMA@/false/"        | \
	sed "s/@CONF_AVAIL_HDF5@/false/"          | \
	sed "s/@CONF_AVAIL_MKL@/false/"           | \
	sed "s/@CONF_AVAIL_BLAS@/false/"          | \
	sed "s/@CONF_AVAIL_LAPACK@/false/"        | \
	sed "s/@CONF_AVAIL_ALGO_SUMMA@/false/"    | \
	sed "s/@CONF_AVAIL_SCALAPACK@/false/"  > $(STATIC_CONFIG_H)


clean :
	rm -f *~
	rm -f *.o
	rm -f algorithm/*.o
	rm -f exception/*.o
	rm -f util/*.o
	rm -f io/*.o
	rm -f $(LIBDASH)
	rm -f $(STATIC_CONFIG_H)


printenv :
	@echo "DART_IMPL   = $(DART_IMPL)"
	@echo "DART_IF     = $(DART_IF)"
	@echo "DASH_ROOT   = $(DASH_ROOT)"
	@echo "LIBDART_DIR = $(LIBDART_DIR)"
	@echo "LIBDART     = $(LIBDART)"
	@echo "DART_INC    = $(DART_INC)"
	@echo "CXX         = $(CXX)"
	@echo "CXXFLAGS    = $(CXXFLAGS)"

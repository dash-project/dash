#
# In-place makefile for use side-by-side with the 
# CMake build system
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



#CXXFLAGS+=-DDASH_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDASH = libdash.a

FILES = Init Team Distribution Math \
	util/Timer util/TimestampClockPosix \
	util/TimestampCounterPosix

OBJS = $(addsuffix .o, $(FILES))

all : $(LIBDASH) 

$(LIBDASH) : $(OBJS)
	ar rcs $(LIBDASH) $(OBJS)

clean : 
	rm -f *~
	rm -f *.o
	rm -f $(LIBDASH)

printenv : 
	@echo "DART_IMPL   = $(DART_IMPL)"
	@echo "DART_IF     = $(DART_IF)"
	@echo "DASH_ROOT   = $(DASH_ROOT)"
	@echo "LIBDART_DIR = $(LIBDART_DIR)"
	@echo "LIBDART     = $(LIBDART)"
	@echo "DART_INC    = $(DART_INC)"
	@echo "CXX         = $(CXX)"
	@echo "CXXFLAGS    = $(CXXFLAGS)"

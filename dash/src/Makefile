#
# In-place makefile for use side-by-side with the 
# CMake build system
#

DASH_ROOT=..
include $(DASH_ROOT)/make.defs

CXXFLAGS+=-I$(DART_INC) -I$(DASH_INC)

#CXXFLAGS+=-DDASH_DEBUG
#CFLAGS+=-D_SVID_SOURCE

LIBDASH = libdash.a

FILES = Init Team Distribution Math

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

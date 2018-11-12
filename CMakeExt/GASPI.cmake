# This module will set the following variables:
#   GASPI_FOUND                  TRUE if we have found GASPI
#   GASPI_COMPILE_FLAGS          Compilation flags for GASPI programs
#   GASPI_INCLUDE_PATH           Include path(s) for GASPI header
#   GASPI_LINK_FLAGS             Linking flags for GASPI programs
#   GASPI_LIBRARY                First GASPI library to link against (cached)

find_library(GASPI_C_LIBRARIES
             NAMES GPI2 libGPI2)

find_path(GASPI_INCLUDE_PATH GASPI.h)

set(CC "clang")
set(GASPI_COMPILE_FLAGS "-Wall -ggdb -O3")
set(GASPI_LINK_FLAGS  "-lpthread")
#"-libverbs"
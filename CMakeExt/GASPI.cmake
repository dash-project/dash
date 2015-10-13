# This module will set the following variables:
#   GASPI_FOUND                  TRUE if we have found GASPI
#   GASPI_COMPILE_FLAGS          Compilation flags for GASPI programs
#   GASPI_INCLUDE_PATH           Include path(s) for GASPI header
#   GASPI_LINK_FLAGS             Linking flags for GASPI programs
#   GASPI_LIBRARY                First GASPI library to link against (cached)

set(GASPI_INCLUDE_PATH "/home/cherold/procs/gpi2/include")
set(GASPI_C_LIBRARIES  "/home/cherold/procs/gpi2/lib/libGPI2.a")
set(GASPI_COMPILE_FLAGS "-Wall")
set(GASPI_LINK_FLAGS "-libverbs" "-lpthread")


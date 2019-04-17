# This module will set the following variables:
#   GASPI_FOUND                  TRUE if we have found GASPI
#   GASPI_CXX_FLAGS              Compilation flags for GASPI programs
#   GASPI_C_LIBRARIES            Libraries for GASPI
#   GASPI_INCLUDE_PATH           Include path(s) for GASPI header
#   GASPI_LINK_FLAGS             Linking flags for GASPI programs


string(REPLACE ":" ";" SEARCH_DIRS "$ENV{LD_LIBRARY_PATH}")

set (GASPI_IMPL_ID "gpi2" CACHE STRING "GASPI implementation identifier")

set (SEARCH_PREFIX "/opt/GPI2" ${SEARCH_DIRS})
#message(STATUS "SEARCHING FOR GPI2:${SEARCH_PREFIX}")

find_path(GASPI_INCLUDE_PATH NAMES GASPI.h PATHS ${SEARCH_PREFIX} PATH_SUFFIXES include)

find_library(GASPI_C_LIBRARIES NAMES libGPI2.a HINTS ${SEARCH_PREFIX} PATH_SUFFIXES lib64 )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GASPI DEFAULT_MSG GASPI_C_LIBRARIES GASPI_INCLUDE_PATH )

mark_as_advanced(
  GASPI_C_LIBRARIES
  GASPI_INCLUDE_PATH
)

set (GASPI_INCLUDE_PATH ${GASPI_INCLUDE_PATH} CACHE STRING "GASPI include path")
set (GASPI_C_LIBRARIES  ${GASPI_C_LIBRARIES}    CACHE STRING "GASPI C libraries")
set (GASPI_LINK_FLAGS   -lGPI2  CACHE STRING "GASPI link flags")
set (GASPI_CXX_FLAGS    -I${GASPI_INCLUDE_PATH} CACHE STRING "GASPI C++ compile flags")

message(STATUS "Detected GASPI library: ${GASPI_IMPL_ID}")

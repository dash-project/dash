# Try to find NUMA libraries and headers.
#
# Usage of this module:
#
#  find_package(NUMA)
#
# Variables defined by this module:
#
#  PAPI_FOUND              System has PAPI libraries and headers
#  PAPI_LIBRARIES          The PAPI library
#  PAPI_INCLUDE_DIRS       The location of PAPI headers

message(STATUS "Searching for NUMA library")

find_library(
  NUMA_LIBRARIES
  NAMES numa
)

find_path(
  NUMA_INCLUDE_DIRS
  NAMES numa.h utmpx.h
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  NUMA DEFAULT_MSG
  NUMA_LIBRARIES
  NUMA_INCLUDE_DIRS
)

mark_as_advanced(
  NUMA_LIBRARIES
  NUMA_INCLUDE_DIRS
)

if (NUMA_FOUND)
  message(STATUS "NUMA includes:  " ${NUMA_INCLUDE_DIRS})
  message(STATUS "NUMA libraries: " ${NUMA_LIBRARIES})
endif()

# Try to find PAPI headers and libraries.
#
# Usage of this module as follows:
#
#  find_package(PAPI)
#
# Variables used by this module, they can change the default behaviour and
# need to be set before calling find_package:
#
#  PAPI_PREFIX             Set this variable to the root installation of
#                          libpapi if the module has problems finding the
#                          proper installation path.
#
# When using module, you can locate the PAPI base path via
#
#  $ module show papi
#
# Variables defined by this module:
#
#  PAPI_FOUND              System has PAPI libraries and headers
#  PAPI_LIBRARIES          The PAPI library
#  PAPI_INCLUDE_DIRS       The location of PAPI headers

# Official PAPI source code repository:
# - git clone https://icl.cs.utk.edu/git/papi.git

find_path(
  PAPI_PREFIX
  NAMES include/papi.h
)

message(STATUS "Searching for PAPI in path " ${PAPI_PREFIX})

find_library(
  PAPI_LIBRARIES
  # Pick the static library first for easier run-time linking.
  NAMES papi
  HINTS ${PAPI_PREFIX}/lib
)

find_path(
  PAPI_INCLUDE_DIRS
  NAMES papi.h
  HINTS ${PAPI_PREFIX}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  PAPI DEFAULT_MSG
  PAPI_LIBRARIES
  PAPI_INCLUDE_DIRS
)

mark_as_advanced(
  PAPI_PREFIX_DIRS
  PAPI_LIBRARIES
  PAPI_INCLUDE_DIRS
)

if (PAPI_FOUND)
  message(STATUS "PAPI includes:  " ${PAPI_INCLUDE_DIRS})
  message(STATUS "PAPI libraries: " ${PAPI_LIBRARIES})
endif()

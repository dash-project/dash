# Try to find nanos libraries and headers.
#
# Variables defined by this module:
#
#  NANOS_FOUND              System has NANOS libraries and headers
#  NANOS_LIBRARIES          The NANOS library
#  NANOS_INCLUDE_DIRS       The location of NANOS headers

find_path(
  NANOS_PREFIX
  NAMES include/nanos.h
)

find_library(
  NANOS_LIBRARIES
  NAMES nanox-c
  HINTS ${NANOS_PREFIX}/lib/performance/
)

find_path(
  NANOS_INCLUDE_DIRS
  NAMES nanos.h
  HINTS ${NANOS_PREFIX}/include/nanox/
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  NANOS DEFAULT_MSG
  NANOS_LIBRARIES
  NANOS_INCLUDE_DIRS
)

mark_as_advanced(
  NANOS_LIBRARIES
  NANOS_INCLUDE_DIRS
)

if (NANOS_FOUND)
  if (NOT $ENV{NANOS_LIB} STREQUAL "")
    set(NANOS_LIBRARIES "$ENV{NANOS_LIB}")
  endif()
  message(STATUS "nanos includes:  " ${NANOS_INCLUDE_DIRS})
  message(STATUS "nanos libraries: " ${NANOS_LIBRARIES})
endif()

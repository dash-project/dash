# Try to find Plasma libraries and headers.
#
# Usage of this module:
#
#  find_package(Plasma)
#
# Variables defined by this module:
#
#  PLASMA_FOUND              System has Plasma libraries and headers
#  PLASMA_LIBRARIES          The Plasma library
#  PLASMA_INCLUDE_DIRS       The location of Plasma headers


if (NOT PLASMA_PREFIX AND NOT $ENV{PLASMA_BASE} STREQUAL "")
  set(PLASMA_PREFIX $ENV{PLASMA_BASE})
endif()

find_path(
  PLASMA_PREFIX
  NAMES include/plasma.h
)

message(STATUS "Searching for Plasma library in path " ${PLASMA_PREFIX})

find_library(
  PLASMA_LIBRARIES
  NAMES plasma
  HINTS ${PLASMA_PREFIX}/lib
)

find_path(
  PLASMA_INCLUDE_DIRS
  NAMES plasma.h
  HINTS ${PLASMA_PREFIX}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  PLASMA DEFAULT_MSG
  PLASMA_LIBRARIES
  PLASMA_INCLUDE_DIRS
)

mark_as_advanced(
  PLASMA_LIBRARIES
  PLASMA_INCLUDE_DIRS
)

if (PLASMA_FOUND)
  if (NOT $ENV{PLASMA_LIB} STREQUAL "")
    set(PLASMA_LIBRARIES "$ENV{PLASMA_LIB}")
  endif()
  message(STATUS "PLASMA includes:  " ${PLASMA_INCLUDE_DIRS})
  message(STATUS "PLASMA libraries: " ${PLASMA_LIBRARIES})
endif()

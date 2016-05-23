# Try to find likwid libraries and headers.
#
# Variables defined by this module:
#
#  LIKWID_FOUND              System has LIKWID libraries and headers
#  LIKWID_LIBRARIES          The LIKWID library
#  LIKWID_INCLUDE_DIRS       The location of LIKWID headers

find_path(
  LIKWID_PREFIX
  NAMES include/likwid.h
)

find_library(
  LIKWID_LIBRARIES
  NAMES likwid
  HINTS ${LIKWID_PREFIX}/lib
)

find_path(
  LIKWID_INCLUDE_DIRS
  NAMES likwid.h
  HINTS ${LIKWID_PREFIX}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  LIKWID DEFAULT_MSG
  LIKWID_LIBRARIES
  LIKWID_INCLUDE_DIRS
)

mark_as_advanced(
  LIKWID_LIBRARIES
  LIKWID_INCLUDE_DIRS
)

if (LIKWID_FOUND)
  if (NOT $ENV{LIKWID_LIB} STREQUAL "")
    set(LIKWID_LIBRARIES "$ENV{LIKWID_LIB}")
  endif()
  message(STATUS "likwid includes:  " ${LIKWID_INCLUDE_DIRS})
  message(STATUS "likwid libraries: " ${LIKWID_LIBRARIES})
endif()

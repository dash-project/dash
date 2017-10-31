# Try to find Ayudame libraries and headers.
#
# Variables defined by this module:
#
#  AYUDAME_FOUND              System has NANOS libraries and headers
#  AYUDAME_LIBRARIES          The NANOS library
#  AYUDAME_INCLUDE_DIRS       The location of NANOS headers


find_path(
  AYUDAME_PREFIX
  NAMES include/ayudame.h
)

find_library(
  AYUDAME_LIBRARIES
  NAMES ayudame
  HINTS ${AYUDAME_PREFIX}/lib/
)

find_path(
  AYUDAME_INCLUDE_DIRS
  NAMES ayudame.h
  HINTS ${AYUDAME_PREFIX}/include/
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  AYUDAME DEFAULT_MSG
  AYUDAME_LIBRARIES
  AYUDAME_INCLUDE_DIRS
)

mark_as_advanced(
  AYUDAME_LIBRARIES
  AYUDAME_INCLUDE_DIRS
)

if (AYUDAME_FOUND)
  if (NOT $ENV{AYUDAME_LIB} STREQUAL "")
    set(AYUDAME_LIBRARIES "$ENV{AYUDAME_LIB}")
  endif()
  message(STATUS "Ayudame includes:  " ${AYUDAME_INCLUDE_DIRS})
  message(STATUS "Ayudame libraries: " ${AYUDAME_LIBRARIES})
endif()

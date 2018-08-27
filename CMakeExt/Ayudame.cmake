# Try to find Ayudame libraries and headers.
#
# Variables defined by this module:
#
#  AYUDAME_FOUND              System has AYUDAME libraries and headers
#  AYUDAME_LIBRARIES          The AYUDAME library
#  AYUDAME_INCLUDE_DIRS       The location of AYUDAME headers

if (ENABLE_AYUDAME)

if (NOT AYUDAME_PREFIX AND NOT $ENV{AYUDAME_ROOT} STREQUAL "")
  set(AYUDAME_PREFIX $ENV{AYUDAME_ROOT})
endif()

message(STATUS "Searching for Ayudame in ${AYUDAME_PREFIX}")

find_path(
  AYUDAME_PREFIX
  NAMES include/ayu_events.h
  HINTS ${AYUDAME_PREFIX}
)

find_library(
  AYUDAME_LIBRARIES
  NAMES ayu_events
  HINTS ${AYUDAME_PREFIX}/lib/
)

find_path(
  AYUDAME_INCLUDE_DIRS
  NAMES ayu_events.h
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
  message(STATUS "Ayudame includes:  " ${AYUDAME_INCLUDE_DIRS})
  message(STATUS "Ayudame libraries: " ${AYUDAME_LIBRARIES})
else()
  message(STATUS "Ayudame not found!")
endif()


endif (ENABLE_AYUDAME)

# Try to find IPM libraries and headers.
#
# Usage of this module:
#
#  find_package(IPM)
#
# Variables defined by this module:
#
#  IPM_FOUND              System has IPM libraries and headers
#  IPM_LIBRARIES          The IPM library
#  IPM_INCLUDE_DIRS       The location of IPM headers

find_path(
  IPM_PREFIX
  NAMES include/ipm.h
)

if (NOT IPM_PREFIX AND NOT $ENV{IPM_HOME} STREQUAL "")
  set(IPM_PREFIX $ENV{IPM_HOME})
endif()
if (NOT IPM_SRC AND NOT $ENV{IPM_SRC} STREQUAL "")
  set(IPM_SRC $ENV{IPM_SRC})
endif()

message(STATUS "Searching for IPM library in path " ${IPM_PREFIX})

find_library(
  IPM_LIBRARIES
  NAMES ipm
  HINTS ${IPM_PREFIX}/lib
)

find_path(
  IPM_INCLUDE_DIRS
  NAMES ipm.h
  HINTS ${IPM_PREFIX}/include
)
find_path(
  IPM_INCLUDE_DIRS
  NAMES ipm.h
  HINTS ${IPM_SRC}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  IPM DEFAULT_MSG
  IPM_LIBRARIES
  IPM_INCLUDE_DIRS
)

mark_as_advanced(
  IPM_LIBRARIES
  IPM_INCLUDE_DIRS
)

if (IPM_FOUND)
  message(STATUS "IPM includes:  " ${IPM_INCLUDE_DIRS})
  message(STATUS "IPM libraries: " ${IPM_LIBRARIES})
endif()

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
  message(INFO "IPM found")
  message(STATUS "IPM includes:  " ${IPM_INCLUDE_DIRS})
  message(STATUS "IPM libraries: " ${IPM_LIBRARIES})
else()
  message(INFO "IPM not found")
endif()

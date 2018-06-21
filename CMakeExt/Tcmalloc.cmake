# Try to find tcmalloc library.
#
# Usage of this module:
#
#  find_package(tcmalloc)
#
# Variables defined by this module:
#
#  TCMALLOC_FOUND              System has tcmalloc libraries and headers
#  TCMALLOC_LIBRARIES          The tcmalloc library

set(TCMALLOC_PREFIX $ENV{TCMALLOC_PREFIX})
message(STATUS "Searching for tcmalloc library in path " ${TCMALLOC_PREFIX})

find_library(
  TCMALLOC_LIBRARIES
  NAMES tcmalloc_minimal
  HINTS ${TCMALLOC_PREFIX}/lib /usr/lib/
)
message(STATUS ${TCMALLOC_LIBRARIES})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  TCMALLOC DEFAULT_MSG
  TCMALLOC_LIBRARIES
)

mark_as_advanced(
  TCMALLOC_LIBRARIES
)

if (TCMALLOC_FOUND)
  message(STATUS "tcmalloc libraries: " ${TCMALLOC_LIBRARIES})
endif()

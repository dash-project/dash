# - Find Required MEMKIND libraries (libvmem, libpmem, libpmemobj)
# This module defines
#  MEMKIND_FOUND
#  MEMKIND_INCLUDE_DIRS, directory containing headers
#  MEMKIND_LIBRARIES, directory containing libraries

if (NOT MEMKIND_PREFIX)
  find_path(
    MEMKIND_PREFIX
    NAMES include/libpmem.h
  )
endif()

if (NOT MEMKIND_PREFIX)
  set(MEMKIND_PREFIX "/usr/")
endif()

message(STATUS "Searching for MEMKIND library in path " ${MEMKIND_PREFIX})

set(MEMKIND_SEARCH_LIB_PATH
  ${MEMKIND_PREFIX}/lib
)
set(MEMKIND_SEARCH_HEADER_PATH
  ${MEMKIND_PREFIX}/include
)

find_path(
  MEMKIND_INCLUDE_DIRS
  NAMES memkind.h
  PATHS ${MEMKIND_SEARCH_HEADER_PATH}
  # make sure we don't accidentally pick up a different version
  NO_DEFAULT_PATH
)

find_library(
  MEMKIND_LIBRARIES
  NAMES memkind
  PATHS ${MEMKIND_SEARCH_LIB_PATH}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  MEMKIND DEFAULT_MSG
  MEMKIND_LIBRARIES
  MEMKIND_INCLUDE_DIRS
)

if (MEMKIND_FOUND)
  if (NOT MEMKIND_FIND_QUIETLY)
    message(STATUS "MEMKIND includes:  " ${MEMKIND_INCLUDE_DIRS})
    message(STATUS "MEMKIND libraries: " ${MEMKIND_LIBRARIES})
  endif()
else()
  if (NOT MEMKIND_FIND_QUIETLY)
    set(MEMKIND_ERR_MSG "Could not find the pmem libraries. Looked for headers")
    set(MEMKIND_ERR_MSG "${MEMKIND_ERR_MSG} in ${MEMKIND_SEARCH_HEADER_PATH}, and for libs")
    set(MEMKIND_ERR_MSG "${MEMKIND_ERR_MSG} in ${MEMKIND_SEARCH_LIB_PATH}")
  endif()
endif()

mark_as_advanced(
  MEMKIND_LIBRARIES
  MEMKIND_INCLUDE_DIRS
)

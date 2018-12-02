# - Find Required PSTL libraries (libvmem, libpmem, libpmemobj)
# This module defines
#  PSTL_FOUND
#  PSTL_INCLUDE_DIRS, directory containing headers
#  PSTL_LIBRARIES, directory containing libraries

if (NOT ENABLE_PSTL)
    return()
else()
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
        string(REGEX MATCH "([0-9]+)" ICC_VERSION_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
        if (CMAKE_MATCH_1 LESS 18)
            message(FATAL_ERROR "Parallel STL requires at least ICC 2018")
        endif()
    else()
        message(WARNING "Parallel STL currently only supported for Intel Compiler")
        return()
    endif()
endif()

if(NOT TBB_FOUND)
    include(${CMAKE_SOURCE_DIR}/CMakeExt/FindTBB.cmake)
endif()

if(NOT TBB_FOUND)
    message(FATAL_ERROR "TBB is required for PSTL")
endif()

if (PSTL_PREFIX)
    message(STATUS "Searching for PSTL in path " ${PSTL_PREFIX})
endif()

# Define search paths based on user input and environment variables
set(PSTL_DEFAULT_SEARCH_DIR "/opt/intel/pstl")
set(PSTL_SEARCH_DIR ${PSTLROOT} ${PSTL_ROOT})

if (DEFINED ENV{INTEL_BASE})
    set(PSTL_SEARCH_DIR ${PSTL_SEARCH_DIR} "$ENV{INTEL_BASE}/linux/pstl")
endif()

find_path(
    PSTL_INCLUDE_DIRS pstl/algorithm
    HINTS ${PSTL_PREFIX} ${PSTL_SEARCH_DIR}
    PATHS ${PSTL_DEFAULT_SEARCH_DIR}
    PATH_SUFFIXES include)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    PSTL DEFAULT_MSG
    PSTL_INCLUDE_DIRS
    )

if (NOT PSTL_FOUND AND NOT PSTL_FIND_QUIETLY)
    set(PSTL_ERR_MSG "Could not find the pmem libraries. Looked for headers")
    set(PSTL_ERR_MSG "${PSTL_ERR_MSG} in ${PSTL_SEARCH_HEADER_PATH}")
endif()

mark_as_advanced(
    PSTL_INCLUDE_DIRS
    )

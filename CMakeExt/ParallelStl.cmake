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

if (NOT PSTL_PREFIX)
    find_path(
        PSTL_PREFIX
        NAMES include/pstl/algorithm
        )
endif()

if (NOT PSTL_PREFIX)
    set(PSTL_PREFIX "/usr/")
endif()

message(STATUS "Searching for PSTL in path " ${PSTL_PREFIX})

set(PSTL_SEARCH_HEADER_PATH
    ${PSTL_PREFIX}/include
    )

find_path(
    PSTL_INCLUDE_DIRS
    NAMES pstl/algorithm
    PATHS ${PSTL_SEARCH_HEADER_PATH}
    # make sure we don't accidentally pick up a different version
    NO_DEFAULT_PATH
    )

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    PSTL DEFAULT_MSG
    PSTL_INCLUDE_DIRS
    )

if (PSTL_FOUND)
    if (NOT PSTL_FIND_QUIETLY)
        message(STATUS "PSTL includes:  " ${PSTL_INCLUDE_DIRS})
    endif()
else()
    if (NOT PSTL_FIND_QUIETLY)
        set(PSTL_ERR_MSG "Could not find the pmem libraries. Looked for headers")
        set(PSTL_ERR_MSG "${PSTL_ERR_MSG} in ${PSTL_SEARCH_HEADER_PATH}")
    endif()
endif()

mark_as_advanced(
    PSTL_INCLUDE_DIRS
    )

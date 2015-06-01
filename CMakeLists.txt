
project(project_dash)

include(${CMAKE_SOURCE_DIR}/CMakeExt/CompilerFlags.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/InstallFiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/FileSystem.cmake)

cmake_minimum_required (VERSION 2.8)

set(DASH_VERSION_MAJOR 0)
set(DASH_VERSION_MINOR 1)
set(DASH_VERSION_PATCH 0)

set(CMAKE_RULE_MESSAGES ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)

if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif(POLICY CMP0054)
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 OLD)
endif(POLICY CMP0053)

# include(CMakeExt/LocateGTest.cmake)
# if (not GTestAvailable)
#   message(status "GoogleTest not found, building without unit tests")
# endif()

if (INSTALL_PREFIX)
  # Install path given in "-DINSTALL_PREFIX=..."
  set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
else()
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
message("-- Install prefix: " ${CMAKE_INSTALL_PREFIX})

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()
message("-- Build type: " ${CMAKE_BUILD_TYPE})

## Build options
option(BUILD_TESTS
       "Whether tests should be built" on)
option(INSTALL_DOCS
       "Whether doxygen docs should be installed" on)
option(WARNINGS_AS_ERRORS
       "Specify whether warnings should be treated as errors" off)
option(BUILD_EXAMPLES
       "Specify whether to build examples" on)

set(DART_IF_VERSION "3.1" CACHE STRING
    "Version of the DART interface")
message("-- Using DART interface v" ${DART_IF_VERSION})

# Include path of selected DART interface:
set(DASH_DART_IF_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include
    CACHE PATH INTERNAL FORCE)
message("-- DART interface include path: "
        ${DASH_DART_IF_INCLUDE_DIR})

## Build results output directories (/bin, /lib, /doc)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc)

## Subprojects, one for each deliverable

set(DART_IMPLEMENTATIONS
    "mpi,shmem" CACHE STRING 
    "Comma-separated list of DASH runtime implementations to
     build, default is mpi,shmem")

string(REPLACE "," ";"
       DART_IMPLEMENTATIONS_LIST
       ${DART_IMPLEMENTATIONS})

# DART interface:
add_subdirectory(dart-if)

# DART implementation of every enabled DART variant:
if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";mpi;")
  set(DART_IMPLEMENTATION_MPI_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/mpi)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";shmem;")
  set(DART_IMPLEMENTATION_SHMEM_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/shmem/dart-shmem)
  add_subdirectory(dart-impl/shmem/dart-shmem-mpi)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";cuda;")
  set(DART_IMPLEMENTATION_CUDA_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/cuda)
endif()

add_subdirectory(dash)


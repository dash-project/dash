
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
  # User given install path given when calling cmake as "-DINSTALL_PREFIX=...".
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
option(BUILD_TESTS "Whether tests should be built" on)
option(INSTALL_DOCS "Whether doxygen docs should be installed" on)
option(WARNINGS_AS_ERRORS "Specify whether warnings should be treated as errors" off)
option(DART_CUDA "Specify whether to build the CUDA implementation of DART" off)

set(DART_IF_VERSION "3.1" CACHE STRING "Version of the DART interface")
message("-- Using DART interface v" ${DART_IF_VERSION})

set(
  DART_IMPLEMENTATION
  "mpi" CACHE STRING "Variant of the DASH runtime implementation (mpi | cuda | shmem)")
set(
  DASH_DART_IF_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include
  CACHE PATH INTERNAL FORCE)
message("-- DART interface include path: " ${DASH_DART_IF_INCLUDE_DIR})

set(
  DART_LIBRARY
  "dart-${DART_IMPLEMENTATION}"
  CACHE STRING INTERNAL FORCE)
set(
  DASH_LIBRARY
  "dash"
  CACHE STRING INTERNAL FORCE)

message(">> DART library: " ${DART_LIBRARY})
message(">> DASH library: " ${DASH_LIBRARY})

## Build results output directories (/bin, /lib, /doc)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc)

## Subprojects, one for each deliverable
add_subdirectory(dart-impl/mpi)
add_subdirectory(dart-impl/shmem/dart-shmem)
add_subdirectory(dart-impl/shmem/dart-shmem-mpi)

if (${DART_CUDA})
  add_subdirectory(dart-impl/cuda)
endif()

add_subdirectory(dash)


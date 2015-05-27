
project(project_dash)

include(${CMAKE_SOURCE_DIR}/CMakeExt/CompilerFlags.cmake)

cmake_minimum_required (VERSION 2.8)

set(DASH_VERSION_MAJOR 0)
set(DASH_VERSION_MINOR 1)
set(DASH_VERSION_PATCH 0)

set(CMAKE_RULE_MESSAGES OFF)

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

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "Release" cache string
  "Choose the type of build, options are: Debug Release." force)
endif(NOT CMAKE_BUILD_TYPE)

## Build options
#
option(BUILD_TESTS "Whether tests should be built" on)
option(INSTALL_DOCS "Whether doxygen docs should be installed" on)
option(WARNINGS_AS_ERRORS "Specify whether warnings should be treated as errors" off)
option(DART_CUDA "Specify whether to build the CUDA implementation of DART" off)

set(DART_IF_VERSION "3.1" CACHE STRING "Version of the DART interface")
message("-- Using DART interface version " ${DART_IF_VERSION} " (change using -DDART_IF_VERSION=<version>)")

## Subprojects, one for each deliverable
#
# add_subdirectory(dart-if)
add_subdirectory(dart-impl/mpi)
add_subdirectory(dart-impl/shmem)

if (${DART_CUDA})
  add_subdirectory(dart-impl/cuda)
endif()

# add_subdirectory(dash)


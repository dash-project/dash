project(project_dash)

include(${CMAKE_SOURCE_DIR}/CMakeExt/InstallFiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/FileSystem.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Doxygen.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Platform.cmake)

cmake_minimum_required (VERSION 2.8)

## Version number
set(DASH_VERSION_MAJOR 0 CACHE STRING "DASH major version number.")
set(DASH_VERSION_MINOR 1 CACHE STRING "DASH minor version number.")
set(DASH_VERSION_PATCH 0 CACHE STRING "DASH patch version number.")
mark_as_advanced(
  DASH_VERSION_MAJOR
  DASH_VERSION_MINOR
  DASH_VERSION_PATCH)

set(DASH_VERSIONED_PROJECT_NAME
    "dash-${DASH_VERSION_MAJOR}.${DASH_VERSION_MINOR}.${DASH_VERSION_PATCH}"
    CACHE STRING INTERNAL FORCE)

set(CMAKE_RULE_MESSAGES ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)

if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif(POLICY CMP0054)
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 OLD)
endif(POLICY CMP0053)

message("-- Platform: " ${DASH_PLATFORM_NAME})
message("-- Platform is POSIX: " ${DASH_PLATFORM_IS_POSIX})

## Install path
if (INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
else()
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
message("-- Install prefix: " ${CMAKE_INSTALL_PREFIX})

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message("-- Build type: " ${CMAKE_BUILD_TYPE})

## Build options
option(BUILD_TESTS
       "Whether tests should be built" on)
option(BUILD_DOCS
       "Whether doxygen documentation should be installed" on)
option(BUILD_EXAMPLES
       "Specify whether to build examples" on)
option(WARNINGS_AS_ERRORS
       "Specify whether warnings should be treated as errors" off)
option(ENABLE_LOGGING
       "Specify whether logging should be enabled" off)
option(ENABLE_TRACE_LOGGING
       "Specify whether trace messages should be logged" off)
option(ENABLE_DART_LOGGING
       "Specify whether messages from DART should be logged" off)
option(ENABLE_ASSERTIONS
       "Specify whether runtime assertions should be checked" off)
option(MEMORY_MODEL_UNIFIED
       "Specify whether to assume unified memory model" off)
message("-- Unified RMA memory model " ${MEMORY_MODEL_UNIFIED})

set(DART_IF_VERSION "3.1" CACHE STRING
    "Version of the DART interface")
message("-- Using DART interface v" ${DART_IF_VERSION})

## Include path of selected DART interface:
set(DASH_DART_IF_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include
    CACHE PATH INTERNAL FORCE)
message("-- DART interface include path: "
        ${DASH_DART_IF_INCLUDE_DIR})

set(DASH_DART_BASE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-impl/base/include
    CACHE PATH INTERNAL FORCE)

## Set compiler flags (depend on CMake options)
include(${CMAKE_SOURCE_DIR}/CMakeExt/CompilerFlags.cmake)

## Build results output directories (/bin, /lib, /doc)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shared)

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

# DART base:
add_subdirectory(dart-impl/base)

# DART implementation of every enabled DART variant:
if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";mpi;")
  set(DART_IMPLEMENTATION_MPI_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/mpi)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";shmem;")
  set(DART_IMPLEMENTATION_SHMEM_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/shmem)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";cuda;")
  set(DART_IMPLEMENTATION_CUDA_ENABLED ON 
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/cuda)
endif()

add_subdirectory(dash)

## Documentation
if (BUILD_DOCS)
  CreateDoxygenTarget()
endif()

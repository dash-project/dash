project(project_dash)

cmake_minimum_required (VERSION 2.8)

# required for Ninja builds
if(POLICY CMP0058)
  cmake_policy(SET CMP0058 NEW)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  set(DEFAULT_COMPTIME_RED on)
else()
  set(DEFAULT_COMPTIME_RED off)
endif()

## Build options
option(BUILD_TESTS
       "Whether tests should be built" on)
option(INSTALL_TESTS
       "Whether tests should be installed" off)
option(BUILD_COVERAGE_TESTS
       "Whether tests are built with code coverage (lcov) support. Implies BUILD_TESTS. Requires GCC." off)
option(BUILD_DOCS
       "Whether doxygen documentation should be installed" off)
option(BUILD_EXAMPLES
       "Specify whether to build examples" on)
option(BUILD_SHARED_LIBS
       "Specify whether libraries should be built as shared objects" off)
option(BUILD_GENERIC
       "Specify whether libraries should built for generic archictecture" off)
option(ENABLE_DEV_COMPILER_WARNINGS
       "Specify whether excessive compiler warnings should be enabled" off)
option(ENABLE_EXT_COMPILER_WARNINGS
       "Specify whether extended compiler warnings should be enabled" off)
option(WARNINGS_AS_ERRORS
       "Specify whether warnings should be treated as errors" off)
option(ENABLE_LT_OPTIMIZATION
       "Specify whether link-time-optimization should be used" off)
option(ENABLE_CC_REPORTS
       "Specify whether compiler reports should be generated" off)
option(ENABLE_COMPTIME_RED
       "Specify whether opts to reduce the build time should be used"
       ${DEFAULT_COMPTIME_RED})
option(ENABLE_THREADSUPPORT
       "Specify whether support for multithreading should be compiled" off)
option(ENABLE_LOGGING
       "Specify whether logging should be enabled" off)
option(ENABLE_TEST_LOGGING
       "Specify whether logging of unit test messages is enabled" on)
option(ENABLE_TRACE_LOGGING
       "Specify whether trace messages should be logged" off)
option(ENABLE_DART_LOGGING
       "Specify whether messages from DART should be logged" off)
option(ENABLE_ASSERTIONS
       "Specify whether runtime assertions should be checked" off)
option(ENABLE_UNIFIED_MEMORY_MODEL
       "Specify whether to assume unified memory model" on)
option(ENABLE_SHARED_WINDOWS
       "Specify whether shared memory features are enabled" on)
option(ENABLE_DYNAMIC_WINDOWS
       "Specify whether to use dynamic MPI windows for collective allocation" on)
option(ENABLE_DEFAULT_INDEX_TYPE_LONG
       "Specify whether to use int64_t as default index type" on)
option(ENABLE_LIBNUMA
       "Specify whether libnuma features are enabled" on)
option(ENABLE_HWLOC
       "Specify whether hwloc features are enabled" on)
option(ENABLE_HWLOC_PCI
       "Specify whether hwloc PCI features are enabled" on)
option(ENABLE_LIKWID
       "Specify whether likwid support is enabled" off)
option(ENABLE_PAPI
       "Specify whether PAPI features are enabled" on)
option(ENABLE_MKL
       "Specify whether MKL features are enabled" on)
option(ENABLE_BLAS
       "Specify whether BLAS features are enabled" on)
option(ENABLE_LAPACK
       "Specify whether LAPACK features are enabled" on)
option(ENABLE_SCALAPACK
       "Specify whether ScaLAPACK features are enabled" on)
option(ENABLE_PLASMA
       "Specify whether PLASMA features are enabled" on)
option(ENABLE_HDF5
       "Specify whether HDF5 features are enabled" on)
option(ENABLE_MEMKIND
       "Specify whether Memkind features are enabled" on)
option(ENABLE_NASTYMPI
       "Specify whether the NastyMPI proxy should be enabled" off)

if (BUILD_COVERAGE_TESTS)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/CodeCoverage.cmake)
  # This has to be done globally as otherwise linking might fail
  append_coverage_compiler_flags()

  set(BUILD_TESTS ON CACHE BOOL
    "Whether tests are built with code coverage (lcov) support.
    Implies BUILD_TESTS. Requires GCC.")
endif()

include(${CMAKE_SOURCE_DIR}/CMakeExt/CMakeEnvCheck.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/MessageColor.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/InstallFiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/FileSystem.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Doxygen.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Platform.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Environment.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/StdLib.cmake)

if (ENABLE_THREADSUPPORT)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/Threading.cmake)
endif()

if (ENABLE_COMPTIME_RED)
    #include(${CMAKE_SOURCE_DIR}/CMakeExt/cotire.cmake)
endif()

# Load build modules to locate libraries after environment setup
# has been loaded:
include(${CMAKE_SOURCE_DIR}/CMakeExt/MPI.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/PAPI.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Hwloc.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Likwid.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/NUMA.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/IPM.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/PLASMA.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/HDF5.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Memkind.cmake)

if (ENABLE_MKL)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/MKL.cmake)
endif()
if (NOT MKL_FOUND)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/LAPACK.cmake)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/SCALAPACK.cmake)
endif()
if (ENABLE_NASTYMPI)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/NastyMPI.cmake)
endif()

# prepare StaticConfig.h generation
include(${CMAKE_SOURCE_DIR}/CMakeExt/GenerateConfig.cmake)

set(DASH_VERSION_MAJOR 0 CACHE STRING "DASH major version number.")
set(DASH_VERSION_MINOR 4 CACHE STRING "DASH minor version number.")
set(DASH_VERSION_PATCH 0 CACHE STRING "DASH patch version number.")
mark_as_advanced(
  DASH_VERSION_MAJOR
  DASH_VERSION_MINOR
  DASH_VERSION_PATCH)
set(DASH_VERSION
    "${DASH_VERSION_MAJOR}.${DASH_VERSION_MINOR}.${DASH_VERSION_PATCH}"
    CACHE STRING INTERNAL FORCE)
set(DASH_VERSIONED_PROJECT_NAME
    "dash-${DASH_VERSION_MAJOR}.${DASH_VERSION_MINOR}.${DASH_VERSION_PATCH}"
    CACHE STRING INTERNAL FORCE)

# check the git commit hash
include (CMakeExt/Gitcommit.cmake)

set(CMAKE_RULE_MESSAGES OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)

## Install path
if (INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
else()
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/opt/")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

## Tests
set(TESTCASES
    "*" CACHE STRING
    "Comma-separated list of test cases to build.
     Default is 'ALL'")

string(REPLACE "," ";"
       TESTCASES_LIST
       ${TESTCASES})

## Subprojects, one for each deliverable

set(DART_IMPLEMENTATIONS
    "mpi" CACHE STRING
    "Comma-separated list of DASH runtime implementations to build.
     Default is 'mpi'")

string(REPLACE "," ";"
       DART_IMPLEMENTATIONS_LIST
       ${DART_IMPLEMENTATIONS})

## Include path of selected DART interface:
set(DASH_DART_IF_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-if/include
    CACHE PATH INTERNAL FORCE)
set(DASH_DART_BASE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-impl/base/include
    CACHE PATH INTERNAL FORCE)

## Set compiler flags (depend on CMake options)
include(${CMAKE_SOURCE_DIR}/CMakeExt/CompilerFlags.cmake)

## Build results output directories (/bin, /lib, /doc)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shared)

# DART interface:
add_subdirectory(dart-if)

# DART base:
add_subdirectory(dart-impl)

add_subdirectory(dash)

include(${CMAKE_SOURCE_DIR}/CMakeExt/GenerateDASHCXX.cmake)

## Documentation
if (BUILD_DOCS)
  CreateDoxygenTarget()
endif()

message(EMPH "DASH version ${DASH_VERSION}")
message(EMPH "-----------------------------------------------------------")
message(EMPH "Install prefix:           (INSTALL_PREFIX)                 "
        ${CMAKE_INSTALL_PREFIX})
message(EMPH "Build type:               (CMAKE_BUILD_TYPE)               "
        ${CMAKE_BUILD_TYPE})
message(EMPH "Generic build:            (BUILD_GENERIC)                  "
        ${BUILD_GENERIC})
message(EMPH "Build shared libraries:   (BUILD_SHARED_LIBS)              "
        ${BUILD_SHARED_LIBS})

message(INFO "Build tests:              (BUILD_TESTS)                    "
        ${BUILD_TESTS})
message(INFO "Host system identifier:   (ENVIRONMENT_TYPE)               "
        ${DASH_ENV_HOST_SYSTEM_ID})
message(INFO "Strict compiler warnings: (ENABLE_DEV_COMPILER_WARNINGS)   "
        ${ENABLE_DEV_COMPILER_WARNINGS})
message(INFO "Style compiler warnings:  (ENABLE_EXT_COMPILER_WARNINGS)   "
        ${ENABLE_EXT_COMPILER_WARNINGS})
message(INFO "Link Time Optimization:   (ENABLE_LT_OPTIMIZATION)         "
        ${ENABLE_LT_OPTIMIZATION})
message(INFO "Compiler reports:         (ENABLE_CC_REPORTS)              "
        ${ENABLE_CC_REPORTS})
message(INFO "Compile time reduction:   (ENABLE_COMPTIME_RED)            "
        ${ENABLE_COMPTIME_RED})
message(INFO "Debug log messages:       (ENABLE_LOGGING)                 "
        ${ENABLE_LOGGING})
message(INFO "Trace log messages:       (ENABLE_TRACE_LOGGING)           "
        ${ENABLE_TRACE_LOGGING})
message(INFO "DART log messages:        (ENABLE_DART_LOGGING)            "
        ${ENABLE_DART_LOGGING})
message(INFO "Runtime assertions:       (ENABLE_ASSERTIONS)              "
        ${ENABLE_ASSERTIONS})
message(INFO "Unified RMA memory model: (ENABLE_UNIFIED_MEMORY_MODEL)    "
        ${ENABLE_UNIFIED_MEMORY_MODEL})
message(INFO "MPI shared windows:       (ENABLE_SHARED_WINDOWS)          "
        ${ENABLE_SHARED_WINDOWS})
message(INFO "MPI dynamic windows:      (ENABLE_DYNAMIC_WINDOWS)         "
        ${ENABLE_DYNAMIC_WINDOWS})
message(INFO "Enable multithreading:    (ENABLE_THREADSUPPORT)           "
        ${ENABLE_THREADSUPPORT})
message(INFO "Default index type long:  (ENABLE_DEFAULT_INDEX_TYPE_LONG) "
        ${ENABLE_DEFAULT_INDEX_TYPE_LONG})
message(INFO "libnuma support:          (ENABLE_LIBNUMA)                 "
        ${ENABLE_LIBNUMA})
message(INFO "hwloc support:            (ENABLE_HWLOC)                   "
        ${ENABLE_HWLOC})
message(INFO "hwloc PCI support:        (ENABLE_HWLOC_PCI)               "
        ${ENABLE_HWLOC_PCI})
message(INFO "PAPI support:             (ENABLE_PAPI)                    "
        ${ENABLE_PAPI})
message(INFO "MKL support:              (ENABLE_MKL)                     "
        ${ENABLE_MKL})
message(INFO "BLAS support:             (ENABLE_BLAS)                    "
        ${ENABLE_BLAS})
message(INFO "LAPACK support:           (ENABLE_LAPACK)                  "
        ${ENABLE_LAPACK})
message(INFO "ScaLAPACK support:        (ENABLE_SCALAPACK)               "
        ${ENABLE_SCALAPACK})
message(INFO "PLASMA support:           (ENABLE_PLASMA)                  "
        ${ENABLE_PLASMA})
message(INFO "HDF5 support:             (ENABLE_HDF5)                    "
        ${ENABLE_HDF5})
message(INFO "Memkind support:          (ENABLE_MEMKIND)                    "
        ${ENABLE_MEMKIND})
message(INFO "Enabled DART backends:    (DART_IMPLEMENTATIONS)           "
        ${DART_IMPLEMENTATIONS})
message(INFO "C   compiler id:          ${CMAKE_C_COMPILER_ID}")
message(INFO "C++ compiler id:          ${CMAKE_CXX_COMPILER_ID}")
if (DEFINED MPI_IMPL_ID)
  message(INFO "MPI implementation:       " ${MPI_IMPL_ID})
endif()

if (ENABLE_PAPI)
  if (PAPI_FOUND)
    message(INFO "PAPI                      enabled")
  else()
    message(NOTE "PAPI                      not found")
  endif()
else()
  message(NOTE "PAPI                      disabled")
endif()
if (ENABLE_MKL)
  if (MKL_FOUND)
    message(INFO "MKL                       enabled")
  else()
    message(NOTE "MKL                       not found")
  endif()
else()
  message(NOTE "MKL                       disabled")
endif()
if (ENABLE_BLAS)
  if (BLAS_FOUND)
    message(INFO "BLAS                      enabled")
  else()
    message(NOTE "BLAS                      not found")
  endif()
else()
  message(NOTE "BLAS                      disabled")
endif()
if (ENABLE_HWLOC)
  if (HWLOC_FOUND)
    message(INFO "hwloc                     enabled")
  else()
    message(NOTE "hwloc                     not found")
  endif()
else()
  message(NOTE "hwloc                     disabled")
endif()
if (ENABLE_LIKWID)
  if (LIKWID_FOUND)
    message(INFO "likwid                    enabled")
  else()
    message(NOTE "likwid                    not found")
  endif()
else()
  message(NOTE "likwid                    disabled")
endif()
if (ENABLE_LAPACK)
  if (LAPACK_FOUND)
    message(INFO "LAPACK                    enabled")
  else()
    message(NOTE "LAPACK                    not found")
  endif()
else()
  message(NOTE "LAPACK                    disabled")
endif()
if (ENABLE_SCALAPACK)
  if (MKL_SCALAPACK_FOUND)
    message(INFO "ScaLAPACK (MKL)           enabled")
  elseif (SCALAPACK_FOUND)
    message(INFO "ScaLAPACK                 enabled")
  else()
    message(NOTE "ScaLAPACK                 not found")
  endif()
else()
  message(NOTE "ScaLAPACK                 disabled")
endif()
if (ENABLE_PLASMA)
  if (PLASMA_FOUND)
    message(INFO "PLASMA                    enabled")
  else()
    message(NOTE "PLASMA                    not found")
  endif()
else()
  message(NOTE "PLASMA                    disabled")
endif()
if (ENABLE_HDF5)
  if (HDF5_FOUND)
    message(INFO "HDF5                      enabled")
  else()
    message(NOTE "HDF5                      not found")
  endif()
else()
  message(NOTE "HDF5                      disabled")
endif()
if (ENABLE_MEMKIND)
  if (MEMKIND_FOUND)
    message(INFO "MEMKIND                   enabled")
  else()
    message(NOTE "MEMKIND                   not found")
  endif()
else()
  message(NOTE "MEMKIND                   disabled")
endif()
if (ENABLE_LIBNUMA)
  if (NUMA_FOUND)
    message(INFO "libnuma                   enabled")
  else()
    message(NOTE "libnuma                   not found")
  endif()
else()
  message(NOTE "libnuma                   disabled")
endif()
if (ENABLE_NASTYMPI)
    message(INFO "NastyMPI                  enabled")
endif()
if (BUILD_TESTS)
  if (GTEST_FOUND)
    message(INFO "googletest                enabled")
  else()
    message(NOTE "googletest                not found")
  endif()
else()
  message(NOTE "googletest                disabled")
endif()

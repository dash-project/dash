find_package(Threads REQUIRED)
include(ExternalProject)


set(GBENCH_LIBRARY_PATH
    "$ENV{GBENCH_LIBRARY_PATH}" CACHE PATH
    "Library path in existing installation of google benchmark")

set(GBENCH_INCLUDE_PATH
    "$ENV{GBENCH_INCLUDE_PATH}" CACHE PATH
    "Include path in existing installation of google benchmark")

set(LIBPREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(LIBSUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")

if (NOT "${GBENCH_LIBRARY_PATH}" STREQUAL "" AND
    NOT "${GBENCH_INCLUDE_PATH}" STREQUAL "")
  # Location of existing gbench installation has been specified:
  set(GBENCH_INCLUDES "${GBENCH_INCLUDE_PATH}")
  set(GBENCH_LIBRARY  "${GBENCH_LIBRARY_PATH}/${LIBPREFIX}benchmark${LIBSUFFIX}")
  message(STATUS "Using existing installation of GBench")
  message(STATUS "gbench      include path: " ${GBENCH_INCLUDES})
  message(STATUS "gbench      library path: " ${GBENCH_LIBRARY})

  add_library(GBench IMPORTED STATIC GLOBAL)
  set_target_properties(
    GBench
    PROPERTIES
    IMPORTED_LOCATION                 "${GBENCH_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
else()
  # Download gbench from official github repository:
  message(STATUS "Downloading GBench from official repository")
  set(GBENCH_PREFIX "${CMAKE_BINARY_DIR}/gbench")
  set(GBENCH_LOCATION "${GBENCH_PREFIX}/src/GBenchExternal-build/src")
  set(GBENCH_INCLUDES "${GBENCH_PREFIX}/src/GBenchExternal/include")
  set(GBENCH_LIBRARY  "${GBENCH_LOCATION}/${LIBPREFIX}benchmark${LIBSUFFIX}")

  set(GBENCH_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE};-DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}")
  # BUILD_BYPRODUCTS not avalable in CMAKE < 3.2.0
  if("${CMAKE_VERSION}" VERSION_LESS 3.2.0)
    ExternalProject_Add(
      GBenchExternal
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG master
      TIMEOUT 10
      PREFIX "${GBENCH_PREFIX}"
      CMAKE_ARGS ${GBENCH_CMAKE_ARGS}
      INSTALL_COMMAND ""
      # Wrap download, configure and build steps in a script to log output
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
    )
  else()
    ExternalProject_Add(
      GBenchExternal
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG master
      TIMEOUT 10
      PREFIX "${GBENCH_PREFIX}"
      CMAKE_ARGS ${GBENCH_CMAKE_ARGS}
      INSTALL_COMMAND ""
      # Necessary for ninja build
      BUILD_BYPRODUCTS ${GBENCH_LIBRARY}
      # Wrap download, configure and build steps in a script to log output
      LOG_DOWNLOAD ON
      LOG_CONFIGURE ON
      LOG_BUILD ON
    )
  endif()
  add_library(GBench IMPORTED STATIC GLOBAL)
  set_target_properties(
    GBench
    PROPERTIES
    IMPORTED_LOCATION                 "${GBENCH_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(GBench GBenchExternal)
endif()


find_package(Threads REQUIRED)
include(ExternalProject)


set(GTEST_LIBRARY_PATH
    "" CACHE STRING
    "Library path in existing installation of gtest")

set(GTEST_INCLUDE_PATH
    "" CACHE STRING
    "Include path in existing installation of gtest")

set(LIBPREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(LIBSUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")

if (NOT "${GTEST_LIBRARY_PATH}" STREQUAL "" AND
    NOT "${GTEST_INCLUDE_PATH}" STREQUAL "")
  # Location of existing gtest installation has been specified:
  set(GTEST_INCLUDES "${GTEST_INCLUDE_PATH}")
  set(GTEST_LIBRARY  "${GTEST_LIBRARY_PATH}/${LIBPREFIX}gtest${LIBSUFFIX}")
  set(GTEST_MAINLIB  "${GTEST_LIBRARY_PATH}/${LIBPREFIX}gtest_main${LIBSUFFIX}")
  message(STATUS "Using existing installation of GTest")
  message(STATUS "gtest      include path: " ${GTEST_INCLUDES})
  message(STATUS "gtest      library path: " ${GTEST_LIBRARY})
  message(STATUS "gtest_main library path: " ${GTEST_MAINLIB})

  add_library(GTest IMPORTED STATIC GLOBAL)
  set_target_properties(
    GTest
    PROPERTIES
    IMPORTED_LOCATION                 "${GTEST_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_library(GTestMain IMPORTED STATIC GLOBAL)
  set_target_properties(
    GTestMain
    PROPERTIES
    IMPORTED_LOCATION                 "${GTEST_MAINLIB}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${GTEST_LIBRARY};${CMAKE_THREAD_LIBS_INIT}")

else()
  # Download gtest from official github repository:
  message(STATUS "Downloading GTest from official repository")
  set(GTEST_PREFIX "${CMAKE_BINARY_DIR}/gtest")
  ExternalProject_Add(
    GTestExternal
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG master
    TIMEOUT 10
    PREFIX "${GTEST_PREFIX}"
    CMAKE_ARGS "-DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}"
    INSTALL_COMMAND ""
    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
  )
  set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build/googlemock/gtest")
  set(GTEST_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/googletest/include")
  set(GTEST_LIBRARY  "${GTEST_LOCATION}/${LIBPREFIX}gtest${LIBSUFFIX}")
  set(GTEST_MAINLIB  "${GTEST_LOCATION}/${LIBPREFIX}gtest_main${LIBSUFFIX}")

  add_library(GTest IMPORTED STATIC GLOBAL)
  set_target_properties(
    GTest
    PROPERTIES
    IMPORTED_LOCATION                 "${GTEST_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(GTest GTestExternal)

  add_library(GTestMain IMPORTED STATIC GLOBAL)
  set_target_properties(
    GTestMain
    PROPERTIES
    IMPORTED_LOCATION                 "${GTEST_MAINLIB}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${GTEST_LIBRARY};${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(GTestMain GTestExternal)
 endif()


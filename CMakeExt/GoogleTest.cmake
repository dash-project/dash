find_package(Threads REQUIRED)
include(ExternalProject)

set(GTEST_PREFIX "${CMAKE_BINARY_DIR}/gtest")
ExternalProject_Add(
  GTestExternal
  SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk
  SVN_REVISION -r HEAD
  TIMEOUT 10
  PREFIX "${GTEST_PREFIX}"
  INSTALL_COMMAND ""
  # Wrap download, configure and build steps in a script to log output
  LOG_DOWNLOAD ON
  LOG_CONFIGURE ON
  LOG_BUILD ON
)

set(LIBPREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
set(LIBSUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")
set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build")
set(GTEST_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/include")
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
  IMPORTED_LOCATION "${GTEST_MAINLIB}"
  IMPORTED_LINK_INTERFACE_LIBRARIES
  "${GTEST_LIBRARY};${CMAKE_THREAD_LIBS_INIT}")
add_dependencies(GTestMain GTestExternal)


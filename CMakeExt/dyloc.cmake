
include(ExternalProject)

# message(STATUS "Looking for dyloc config in ${DYLOC_BASE}")
# find_package(dyloc CONFIG REQUIRED HINTS ${DYLOC_BASE})

if(ENABLE_DYLOC)
  message(STATUS "Building dyloc")
  set(DYLOC_PREFIX "${CMAKE_BINARY_DIR}/dyloc")

  list(
    APPEND DYLOC_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/external
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DEXTRA_C_FLAGS=${PCM_EXTRA_C_FLAGS}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DEXTRA_CXX_FLAGS=${PCM_EXTRA_CXX_FLAGS}
    -DPARENT_INCLUDE_DIR=${PROJECT_BINARY_DIR}/external/include
    -DPARENT_DEFINITIONS=${PARENT_DEFINITIONS}
    -DPARENT_BINARY_DIR=${PROJECT_BINARY_DIR}
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDE_DIRS}
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARIES}
    -DBUILD_TESTS=${DYLOC_TESTS}
  )
  ExternalProject_Add(
    dylocExternal
    GIT_REPOSITORY https://github.com/dash-project/dyloc.git
    GIT_TAG master
    TIMEOUT 10
    PREFIX "${DYLOC_PREFIX}"
    CMAKE_ARGS ${DYLOC_CMAKE_ARGS}
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
  )
  set(DYLOC_LOCATION "${DYLOC_PREFIX}/src/GTestExternal-build/googlemock/gtest")
  set(DYLOC_INCLUDES "${DYLOC_PREFIX}/src/GTestExternal/googletest/include")
  set(DYLOC_LIBRARY  "${DYLOC_LOCATION}/${LIBPREFIX}gtest${LIBSUFFIX}")
  set(DYLOC_MAINLIB  "${DYLOC_LOCATION}/${LIBPREFIX}gtest_main${LIBSUFFIX}")

  add_library(dylocxx IMPORTED STATIC GLOBAL)
  set_target_properties(
    dylocxx
    PROPERTIES
    IMPORTED_LOCATION                 "${DYLOC_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(dylocxx dylocExternal)

  add_library(dyloc IMPORTED STATIC GLOBAL)
  set_target_properties(
    dyloc
    PROPERTIES
    IMPORTED_LOCATION                 "${DYLOC_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(dyloc dylocExternal)

endif()


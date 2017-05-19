
include(ExternalProject)

# message(STATUS "Looking for dyloc config in ${DYLOC_BASE}")
# find_package(dyloc CONFIG REQUIRED HINTS ${DYLOC_BASE})

if(ENABLE_DYLOC)
  set(DART_IMPLEMENTATIONS ${DART_IMPLEMENTATIONS} PARENT_SCOPE)

  set(DYLOC_PREFIX         "${CMAKE_BINARY_DIR}/dyloc")
# set(DYLOC_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(DYLOC_INSTALL_PREFIX "${DYLOC_PREFIX}/src/dylocExternal/dyloc")

  message(STATUS "Building dyloc in ${DYLOC_PREFIX}")

  list(
    APPEND DYLOC_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${DYLOC_INSTALL_PREFIX}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDE_DIRS}
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARIES}
    -DBUILD_TESTS=OFF
    -DENABLE_PAPI=${ENABLE_PAPI}
    -DENABLE_LIBNUMA=${ENABLE_LIBNUMA}
    -DDART_IMPLEMENTATIONS=${DART_IMPLEMENTATIONS}
    -DDART_PREFIX=${CMAKE_BINARY_DIR}
    -DDART_INCLUDE_DIRS=${CMAKE_SOURCE_DIR}/dart-if/include
  )
  ExternalProject_Add(
    dylocExternal
    GIT_REPOSITORY https://github.com/dash-project/dyloc.git
    GIT_TAG master
    TIMEOUT 10
    PREFIX "${DYLOC_PREFIX}"
    CMAKE_ARGS ${DYLOC_CMAKE_ARGS}
  # INSTALL_COMMAND ""
    INSTALL_DIR ${DYLOC_INSTALL_PREFIX}
    LOG_INSTALL ON
  )
  set(DYLOC_LOCATION       "${DYLOC_PREFIX}/src/dylocExternal-build/dyloc")
  set(DYLOC_INCLUDES       "${DYLOC_PREFIX}/src/dylocExternal/dyloc/include")
  set(DYLOC_LIBRARY        "${DYLOC_LOCATION}/${LIBPREFIX}dyloc${LIBSUFFIX}")
  set(DYLOC_COMMON_LIBRARY "${DYLOC_LOCATION}/${LIBPREFIX}dyloc-common${LIBSUFFIX}")
  set(DYLOCXX_LIBRARY      "${DYLOC_LOCATION}/${LIBPREFIX}dylocxx${LIBSUFFIX}")

  message(STATUS "dyloc      location:     " ${DYLOC_LOCATION})
  message(STATUS "dyloc      include path: " ${DYLOC_INCLUDES})

  add_dependencies(dylocExternal dart-mpi)
  add_dependencies(dylocExternal dart-base)

  add_library(dyloc-common IMPORTED STATIC GLOBAL)
  set_target_properties(
    dyloc-common
    PROPERTIES
    IMPORTED_LOCATION                 "${DYLOC_LIBRARY}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
  add_dependencies(dyloc-common dylocExternal)

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


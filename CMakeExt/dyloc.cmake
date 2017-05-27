
include(ExternalProject)

# Discussion of integration alternatives:
# https://stackoverflow.com/questions/9689183/cmake-googletest


set(DYLOC_SOURCE_PATH
    "$ENV{DYLOC_SOURCE_PATH}" CACHE PATH
    "Base path to the dyloc source distribution")

if(ENABLE_DYLOC)
  set(DART_IMPLEMENTATIONS ${DART_IMPLEMENTATIONS} PARENT_SCOPE)
  set(ENVIRONMENT_TYPE     ${ENVIRONMENT_TYPE}     PARENT_SCOPE)
  set(ENABLE_LOGGING       ${ENABLE_LOGGING}       PARENT_SCOPE)

  set(DYLOC_PREFIX         "${CMAKE_BINARY_DIR}/dyloc")
  set(DYLOC_INSTALL_PREFIX "${DYLOC_PREFIX}/install")

  message(STATUS "Building dyloc in ${DYLOC_PREFIX}")

  list(
    APPEND DYLOC_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DENVIRONMENT_TYPE=${ENVIRONMENT_TYPE}
    -DCMAKE_INSTALL_PREFIX=${DYLOC_INSTALL_PREFIX}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DBOOST_INCLUDEDIR=${BOOST_INCLUDE_DIRS}
    -DBOOST_LIBRARYDIR=${BOOST_LIBRARIES}
    -DBUILD_TESTS=OFF
    -DENABLE_LOGGING=${ENABLE_LOGGING}
    -DENABLE_PAPI=${ENABLE_PAPI}
    -DENABLE_LIBNUMA=${ENABLE_LIBNUMA}
    -DDART_IMPLEMENTATIONS=${DART_IMPLEMENTATIONS}
    -DDART_PREFIX=${CMAKE_BINARY_DIR}
    -DDART_INCLUDE_DIRS=${CMAKE_SOURCE_DIR}/dart-if/include
  )
  if (DYLOC_SOURCE_PATH)
    ExternalProject_Add(
      dylocExternal
      URL ${DYLOC_SOURCE_PATH}
      PREFIX "${DYLOC_PREFIX}"
      CMAKE_ARGS ${DYLOC_CMAKE_ARGS}
      INSTALL_DIR ${DYLOC_INSTALL_PREFIX}
      LOG_INSTALL ON
    )
  else()
    ExternalProject_Add(
      dylocExternal
      GIT_REPOSITORY https://github.com/dash-project/dyloc.git
      GIT_TAG master
      TIMEOUT 10
      PREFIX "${DYLOC_PREFIX}"
      CMAKE_ARGS ${DYLOC_CMAKE_ARGS}
      INSTALL_DIR ${DYLOC_INSTALL_PREFIX}
      LOG_INSTALL ON
    )
  endif()

  set(DYLOC_BUILD_DIR      "${DYLOC_PREFIX}/src/dylocExternal-build/dyloc")
  set(DYLOC_BASE           "${DYLOC_INSTALL_PREFIX}")
  set(DYLOC_INCLUDES       "${DYLOC_BASE}/include")
  set(DYLOC_LIBRARY        "${DYLOC_BASE}/lib/${LIBPREFIX}dyloc${LIBSUFFIX}")
  set(DYLOC_COMMON_LIBRARY "${DYLOC_BASE}/lib/${LIBPREFIX}dyloc-common${LIBSUFFIX}")
  set(DYLOCXX_LIBRARY      "${DYLOC_BASE}/lib/${LIBPREFIX}dylocxx${LIBSUFFIX}")

  message(STATUS "dyloc      source path:  " ${DYLOC_SOURCE_PATH})
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


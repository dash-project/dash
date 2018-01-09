find_package(Threads REQUIRED)
include(ExternalProject)

set (GTEST_GIT_TAG "be6ee26a9b5b814c3e173c6e5e97c385fbdc1fb0")

if (BUILD_TESTS)

  set(GTEST_LIBRARY_PATH
      "$ENV{GTEST_LIBRARY_PATH}" CACHE PATH
      "Library path in existing installation of gtest")

  set(GTEST_INCLUDE_PATH
      "$ENV{GTEST_INCLUDE_PATH}" CACHE PATH
      "Include path in existing installation of gtest")

  set(LIBPREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
  set(LIBSUFFIX "${CMAKE_STATIC_LIBRARY_SUFFIX}")

  if (NOT "${GTEST_LIBRARY_PATH}" STREQUAL "" AND
      NOT "${GTEST_INCLUDE_PATH}" STREQUAL "")
    # Location of existing gtest installation has been specified:
    set(GTEST_INCLUDES "${GTEST_INCLUDE_PATH}" PARENT_SCOPE)
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
    message(STATUS "GoogleTest: using GTest from official repository")
    set(GTEST_SUBMOD "${CMAKE_SOURCE_DIR}/vendor/googletest")
    set(GTEST_PREFIX "${CMAKE_BINARY_DIR}/gtest")
    set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build/googlemock/gtest")
    set(GTEST_INCLUDES "${GTEST_SUBMOD}/googletest/include")
    set(GTEST_LIBRARY  "${GTEST_LOCATION}/${LIBPREFIX}gtest${LIBSUFFIX}")
    set(GTEST_MAINLIB  "${GTEST_LOCATION}/${LIBPREFIX}gtest_main${LIBSUFFIX}")

    # check whether the submodule has been initialized
    file(GLOB gtest_files "${GTEST_SUBMOD}/*")
    list(LENGTH gtest_files gtest_file_cnt)
    set (git_res 0)
    if (gtest_file_cnt EQUAL 0)
      # we first need to initialize the submodule
      message(STATUS "GoogleTest: Fetching submodule into vendor/googletest")
      execute_process(COMMAND git submodule update --init vendor/googletest
                      TIMEOUT 10
                      RESULT_VARIABLE git_res
                      OUTPUT_VARIABLE git_out
                      ERROR_VARIABLE  git_err
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    if (git_res EQUAL 0)
      # checkout desired version
      message(STATUS "GoogleTest: Checking out commit ${GTEST_GIT_TAG}")
      execute_process(COMMAND git checkout -q ${GTEST_GIT_TAG}
                      RESULT_VARIABLE git_res
                      OUTPUT_VARIABLE git_out
                      ERROR_VARIABLE  git_err
                      WORKING_DIRECTORY ${GTEST_SUBMOD})
      if (NOT git_res EQUAL 0)
        # try to fetch latest commit from origin
        message(STATUS "GoogleTest: Checkout failed, trying to fetch latest commit")
        execute_process(COMMAND git submodule update --remote vendor/googletest
                        TIMEOUT 10
                        RESULT_VARIABLE git_res
                        OUTPUT_VARIABLE git_out
                        ERROR_VARIABLE  git_err
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        if (git_res EQUAL 0)
          message(STATUS "GoogleTest: Checking out commit ${GTEST_GIT_TAG}")
          execute_process(COMMAND git checkout -q ${GTEST_GIT_TAG}
                          RESULT_VARIABLE git_res
                          OUTPUT_VARIABLE git_out
                          ERROR_VARIABLE  git_err
                          WORKING_DIRECTORY ${GTEST_SUBMOD})
        endif()
        if (NOT git_res EQUAL 0)
          message(FATAL_ERROR "GoogleTest: Failed to checkout commit ${GTEST_GIT_TAG}: ${git_err}")
        endif()
      endif()

      # BUILD_BYPRODUCTS not avalable in CMAKE < 3.2.0
      # TODO: do we still need to set BUILD_BYPRODUCTS?
      if("${CMAKE_VERSION}" VERSION_LESS 3.2.0)
        ExternalProject_Add(
          GTestExternal
          SOURCE_DIR ${GTEST_SUBMOD}
          PREFIX "${GTEST_PREFIX}"
          CMAKE_ARGS "-DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}"
          INSTALL_COMMAND ""
          # Wrap configure and build steps in a script to log output
          LOG_CONFIGURE ON
          LOG_BUILD ON
        )
      else()
        ExternalProject_Add(
          GTestExternal
          SOURCE_DIR ${GTEST_SUBMOD}
          PREFIX "${GTEST_PREFIX}"
          CMAKE_ARGS "-DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER};-DCMAKE_CXX_COMPILER:string=${CMAKE_CXX_COMPILER}"
          INSTALL_COMMAND ""
          # Necessary for ninja build
          BUILD_BYPRODUCTS ${GTEST_LIBRARY}
          # Wrap configure and build steps in a script to log output
          LOG_CONFIGURE ON
          LOG_BUILD ON
        )
      endif()

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

      set (GTEST_FOUND 1)
      set (GTEST_FOUND 1 PARENT_SCOPE)

    else ()
      message(STATUS "GoogleTest: Failed to update submodule, disabling tests")
      set (GTEST_FOUND 0)
      set (GTEST_FOUND 0 PARENT_SCOPE)
    endif()

  endif()

endif (BUILD_TESTS)

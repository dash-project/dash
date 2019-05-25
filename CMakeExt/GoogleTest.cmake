find_package(Threads REQUIRED)

set (GTEST_GIT_TAG "HEAD")

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
    set(GTEST_INCLUDES "${GTEST_INCLUDE_PATH}")
    set(GTEST_LIBRARY  "${GTEST_LIBRARY_PATH}/${LIBPREFIX}gtest${LIBSUFFIX}")
    set(GTEST_MAINLIB  "${GTEST_LIBRARY_PATH}/${LIBPREFIX}gtest_main${LIBSUFFIX}")
    message(STATUS "Attempting to use existing installation of GTest")
    message(STATUS "gtest      include path: " ${GTEST_INCLUDES})
    message(STATUS "gtest      library path: " ${GTEST_LIBRARY})
    message(STATUS "gtest_main library path: " ${GTEST_MAINLIB})

    find_path(GTEST_HEADER "gtest.h" ${GTEST_INCLUDE_PATH}/gtest)
    if (NOT GTEST_HEADER OR NOT (EXISTS ${GTEST_LIBRARY} AND EXISTS ${GTEST_MAINLIB}))
      message(WARNING "Cannot use user-supplied GoogleTest directory")
    else ()
      set (GTEST_FOUND 1)
      set (GTEST_FOUND 1 PARENT_SCOPE)
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
     endif()

  else()
    # Download gtest from official github repository:
    set(GTEST_SUBMOD "${CMAKE_SOURCE_DIR}/vendor/googletest")
    set(GTEST_PREFIX "${CMAKE_BINARY_DIR}/gtest")
    set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build/googlemock/gtest")
    set(GTEST_INCLUDES "${GTEST_SUBMOD}/googletest/include")
    set(GTEST_LIBRARY  "gtest")
    set(GTEST_MAINLIB  "gtest_main")

    # check whether the submodule has been initialized
    file(GLOB gtest_files     "${GTEST_SUBMOD}/*")
    list(LENGTH gtest_files     gtest_file_cnt)
    set (git_res 0)
    if (gtest_file_cnt EQUAL 0)
      # we first need to initialize the submodule
      message(STATUS "GoogleTest: using GTest from official repository")
      message(STATUS "GoogleTest: Fetching submodule into ${GTEST_SUBMOD}")
      execute_process(COMMAND git submodule update --init ${GTEST_SUBMOD}
                      TIMEOUT 10
                      RESULT_VARIABLE git_res
                      OUTPUT_VARIABLE git_out
                      ERROR_VARIABLE  git_err
                      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    file(GLOB gtest_git_files "${GTEST_SUBMOD}/.git*")
    list(LENGTH gtest_git_files gtest_git_file_cnt)
    if (NOT gtest_git_file_cnt EQUAL 0 AND git_res EQUAL 0)
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
        execute_process(COMMAND git submodule update --remote ${GTEST_SUBMOD}
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
          message(WARNING "GoogleTest: Failed to checkout commit ${GTEST_GIT_TAG}: ${git_err}")
        endif()
      endif()
    elseif (NOT gtest_file_cnt EQUAL 0)
      message(STATUS "GoogleTest: using GTest available in ${GTEST_SUBMOD}")
    endif()


    if (git_res EQUAL 0)
      # remove -Werror flag for googletest
      #string(REPLACE " -Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      #string(REPLACE "-Wmissing-include-dirs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
      add_subdirectory(../vendor/googletest/googletest ${PROJECT_BINARY_DIR}/testing)
      # add the flag again
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-include-dirs")


      set (GTEST_FOUND 1)
      set (GTEST_FOUND 1 PARENT_SCOPE)
    else ()
      message(WARNING "GoogleTest: Failed to update submodule, disabling tests\n"
                      "            GIT returned ${git_res}\n"
                      "            GIT stdout: ${git_out} \n"
                      "            GIT stderr: ${git_err}")
      set (GTEST_FOUND 0)
      set (GTEST_FOUND 0 PARENT_SCOPE)
    endif()

  endif()

endif (BUILD_TESTS)

project(project_dart_gaspi_test CXX)

# Load global build settings
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR}
    PARENT_SCOPE)
set(BUILD_TESTS ${BUILD_TESTS}
    PARENT_SCOPE)
set(GTEST_INCLUDE_PATH ${GTEST_INCLUDE_PATH}
    PARENT_SCOPE)
set(GTEST_LIBRARY_PATH ${GTEST_LIBRARY_PATH}
    PARENT_SCOPE)

# TODO set include and link path from gtest flags
set(ADDITIONAL_INCLUDES      ${GTEST_INCLUDE_PATH})
set(ADDITIONAL_LIBRARIES     ${GTEST_LIBRARY})

include(${CMAKE_SOURCE_DIR}/CMakeExt/GoogleTest.cmake)



message(INFO "Building dart unit tests with backend " "gaspi")

set(DART_TEST_CASE_LIST test_01_hello
                        test_02_group
                        test_03_barrier
                        test_03_teams
                        test_04_gptr
                        test_05_mem_local
                        test_05_memory
                        test_05_mem_team
                        test_06_allgather
                        test_06_bcast
                        test_06_gather
                        test_06_scatter
                        test_07_get_blocking
                        test_07_get_gptr
                        test_07_get_gptr_blocking
                        test_07_get_gptr_handle
                        test_07_notify_put
                        test_07_notify_put_handle
                        test_08_lock)

foreach(dart_test_case ${DART_TEST_CASE_LIST})
    set(DART_TEST ${dart_test_case})
    set(DART_LIBRARY "dart-gaspi")

    # Source- and header files to be compiled (OBJ):
    file(GLOB_RECURSE DART_TEST_SOURCES
        "${dart_test_case}/*.hpp"
        "${dart_test_case}/*.h"
        "${dart_test_case}/*.cc"
        "${dart_test_case}/*.cpp")

    if(DART_TEST_SOURCES STREQUAL "")
        continue()
    endif()

    include_directories(
        ${CMAKE_SOURCE_DIR}/dash/include
        ${ADDITIONAL_INCLUDES}
    )

    add_executable(
      ${DART_TEST}
      ${DART_TEST_SOURCES}
    )
    target_link_libraries(
      ${DART_TEST}
      GTest
      ${DART_LIBRARY}
      ${ADDITIONAL_LIBRARIES}
    )
    set_target_properties(
      ${DART_TEST} PROPERTIES
      CXX_STANDARD 11
    )
    set_target_properties(
      ${DART_TEST} PROPERTIES
      CXX_STANDARD_REQUIRED 11
    )
      # Installation
    DeployBinary(${DART_TEST})
    install(
      TARGETS ${DART_TEST}
      DESTINATION bin/dart/test/gaspi)
endforeach(dart_test_case ${DART_TEST_CASE_LIST})

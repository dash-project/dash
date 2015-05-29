project(project_dash_test)

set(DART_LIB libdart-mpi)
set(DART_LIB libdash)

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} PARENT_SCOPE)

SubDirList(DASH_TEST_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})
foreach(testname ${DASH_TEST_DIRECTORIES})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${testname})
    add_executable(${testname} ${testname}/main.cpp)
    include_directories(
      ${CMAKE_SOURCE_DIR}/dash/include
      ${DASH_DART_IF_INCLUDE_DIR}
    )
    target_link_libraries(
      ${testname}
      ${DASH_LIB}
      ${DART_LIB})
    DeployBinary(${testname})
    install(TARGETS ${testname} DESTINATION bin/dash/test)
  endif()
endforeach(testname ${DASH_TEST_DIRECTORIES})


project(project_dash_libdash)

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} PARENT_SCOPE)
set(DASH_DART_IF_INCLUDE_DIR ${DART_LIBRARY} PARENT_SCOPE)
set(DASH_DART_IF_INCLUDE_DIR ${DASH_LIBRARY} PARENT_SCOPE)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_LIBRARY_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_LIBRARY_HEADERS "include/*.h")

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_LIBRARY_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
include_directories(
  ${DASH_LIBRARY_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
)
# Library compilation sources
add_library(
  ${DASH_LIBRARY}         # library name
  ${DASH_LIBRARY_SOURCES} # sources
  ${DASH_LIBRARY_HEADERS} # headers
)
target_link_libraries(
  ${DASH_LIBRARY} # library name
  ${DART_LIBRARY} # dep
)
DeployLibrary(${DASH_LIBRARY})

## Tests

SubDirList(DASH_TEST_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/test)
foreach(testname ${DASH_TEST_DIRECTORIES})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test/${testname})
    add_executable(${testname} test/${testname}/main.cpp)
    include_directories(
      ${CMAKE_SOURCE_DIR}/dash/include
    )
    target_link_libraries(
      ${testname}
      ${DASH_LIBRARY}
      ${DART_LIBRARY})
    DeployBinary(${testname})
    install(TARGETS ${testname} DESTINATION bin/dash/test)
  endif()
endforeach(testname ${DASH_TEST_DIRECTORIES})

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_LIBRARY} DESTINATION lib)


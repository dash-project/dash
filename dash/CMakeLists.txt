project(project_dash_libdash)

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} PARENT_SCOPE)
set(DASH_DART_IF_INCLUDE_DIR ${DART_LIBRARY} PARENT_SCOPE)
set(DASH_DART_IF_INCLUDE_DIR ${DASH_LIBRARY} PARENT_SCOPE)
set(BUILD_EXAMPLES ${BUILD_EXAMPLES} PARENT_SCOPE)

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

## Examples

if (BUILD_EXAMPLES)
  SubDirList(DASH_TEST_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/examples)
  foreach(examplename ${DASH_TEST_DIRECTORIES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/${examplename})
      add_executable(${examplename} examples/${examplename}/main.cpp)
      include_directories(
        ${CMAKE_SOURCE_DIR}/dash/include
      )
      target_link_libraries(
        ${examplename}
        ${DASH_LIBRARY}
        ${DART_LIBRARY})
      DeployBinary(${examplename})
      install(TARGETS ${examplename} DESTINATION bin/dash/example)
    endif()
  endforeach(examplename ${DASH_TEST_DIRECTORIES})
endif()
## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_LIBRARY} DESTINATION lib)


project(project_dart_base)

# Library name
set(DASH_DART_BASE_LIBRARY dart-base)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_BASE_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_BASE_HEADERS "include/*.h")

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} 
    PARENT_SCOPE)

set(DASH_DART_BASE_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include_directories(
  ${DASH_DART_BASE_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
)

add_library(
  ${DASH_DART_BASE_LIBRARY} # library name
  STATIC
  ${DASH_DART_BASE_SOURCES} # sources
  ${DASH_DART_BASE_HEADERS} # headers
)

## Installation

DeployLibrary(${DASH_DART_BASE_LIBRARY})

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")

# Install library
install(TARGETS ${DASH_DART_BASE_LIBRARY} DESTINATION lib)

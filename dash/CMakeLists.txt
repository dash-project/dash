project(project_dash_libdash)

# Extra flags
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_SVID_SOURCE")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SVID_SOURCE")

# Library name
set(DASH_LIBRARY dash)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_LIBRARY_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_LIBRARY_HEADERS "include/*.h")

# Include directory to selected version of DART interface
set(DASH_DART_IF_INCLUDE_DIR
   ${CMAKE_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include)

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
  ${DASH_LIBRARY} # library name
  ${DASH_LIBRARY_SOURCES} # sources
  ${DASH_LIBRARY_HEADERS} # headers
)
DeployLibrary(${DASH_LIBRARY})

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_DART_IMPL_SHMEM_LIBRARY} DESTINATION lib)


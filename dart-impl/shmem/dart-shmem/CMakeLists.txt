project(project_dash_dart_impl_shmem)

# Extra flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_HELPER_THREAD=1 -DUSE_EVENTFD -D_SVID_SOURCE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_HELPER_THREAD=1 -DUSE_EVENTFD -D_SVID_SOURCE")

# Library name
set(DASH_DART_IMPL_SHMEM_LIBRARY dart-shmem)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_IMPL_SHMEM_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_IMPL_SHMEM_HEADERS "include/*.h")

# Include directory to selected version of DART interface
set(DASH_DART_IF_INCLUDE_DIR
   ${CMAKE_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include)

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_DART_IMPL_SHMEM_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include_directories(
  ${DASH_DART_IMPL_SHMEM_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
)

# Library compilation sources
add_library(
  ${DASH_DART_IMPL_SHMEM_LIBRARY} # library name
  ${DASH_DART_IMPL_SHMEM_SOURCES} # sources
  ${DASH_DART_IMPL_SHMEM_HEADERS} # headers
)

add_executable(
  dartrun-shmem
  src/dartrun.c
  ${DASH_DART_IMPL_SHMEM_HEADERS} # headers
)

target_link_libraries(
  dartrun-shmem
  ${DASH_DART_IMPL_SHMEM_LIBRARY} # library name
  pthread
)

# Link dependencies
# target_link_libraries(
#   ${DASH_DART_IMPL_SHMEM_LIBRARY}
#   <dependencies>
# )

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_DART_IMPL_SHMEM_LIBRARY} DESTINATION lib)


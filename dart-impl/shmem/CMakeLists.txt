project(project_dash_dart_impl_shmem C)

# Extra flags
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -DUSE_HELPER_THREAD=1 -DUSE_EVENTFD -D_SVID_SOURCE -pthread")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DUSE_HELPER_THREAD=1 -DUSE_EVENTFD -D_SVID_SOURCE -pthread")
set(ENABLE_LOGGING ${ENABLE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_DART_LOGGING ${ENABLE_DART_LOGGING}
    PARENT_SCOPE)

# Library name
set(DASH_LIBRARY ${DASH_LIBRARY} PARENT_SCOPE)
set(DASH_DART_IMPL_SHMEM_LIBRARY dart-shmem)
set(DARTRUN_BINARY dartrun-shmem)
set(DASH_DART_BASE_LIBRARY dart-base)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_IMPL_SHMEM_SOURCES
     "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_IMPL_SHMEM_HEADERS
     "include/*.h")

# Include directory to selected version of DART interface
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR}
    PARENT_SCOPE)

## Configure compile flags

set (ADDITIONAL_COMPILE_FLAGS "-DDART")

if (ENABLE_DART_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LOGGING")
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} -DDART_ENABLE_LOGGING")
endif()

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_DART_IMPL_SHMEM_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include_directories(
  ${DASH_DART_IMPL_SHMEM_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
  ${DASH_DART_BASE_INCLUDE_DIR}
)

# Library compilation sources
add_library(
  ${DASH_DART_IMPL_SHMEM_LIBRARY} # library name
# SHARED                          # SHARED for shared library
  ${DASH_DART_IMPL_SHMEM_SOURCES} # sources
  ${DASH_DART_IMPL_SHMEM_HEADERS} # headers
)
target_link_libraries(
  ${DASH_DART_IMPL_SHMEM_LIBRARY} # library name
  ${DASH_DART_BASE_LIBRARY}
  pthread
)

set_target_properties(
  ${DASH_DART_IMPL_SHMEM_LIBRARY} PROPERTIES
  COMPILE_FLAGS ${ADDITIONAL_COMPILE_FLAGS}
)

DeployLibrary(${DASH_DART_IMPL_SHMEM_LIBRARY})

add_executable(
  ${DARTRUN_BINARY}
  src/dartrun.c
  ${DASH_DART_IMPL_SHMEM_HEADERS} # headers
)
target_link_libraries(
  ${DARTRUN_BINARY}
  ${DASH_DART_IMPL_SHMEM_LIBRARY} # library name
  pthread
)
DeployBinary(${DARTRUN_BINARY})

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_DART_IMPL_SHMEM_LIBRARY} DESTINATION lib)
# Binary
install(TARGETS ${DARTRUN_BINARY} DESTINATION bin)


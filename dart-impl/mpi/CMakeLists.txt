project(project_dash_dart_impl_mpi C)

# Library name
set(DASH_DART_IMPL_MPI_LIBRARY dart-mpi)

set(DASH_DART_BASE_LIBRARY dart-base)

include(${CMAKE_SOURCE_DIR}/CMakeExt/MPI.cmake)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_IMPL_MPI_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_IMPL_MPI_HEADERS "include/*.h")

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} 
    PARENT_SCOPE)
set(ENABLE_DART_LOGGING ${ENABLE_DART_LOGGING}
    PARENT_SCOPE)
set(MEMORY_MODEL_UNIFIED ${MEMORY_MODEL_UNIFIED}
    PARENT_SCOPE)

message("-- MPI include path: " "${MPI_INCLUDE_PATH}")
message("-- MPI libraries:    " "${MPI_C_LIBRARIES}")
message("-- MPI link flags:   " "${MPI_LINK_FLAGS}")

## Configure compile flags

set (ADDITIONAL_COMPILE_FLAGS "-DDART")

if (ENABLE_DART_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LOGGING")
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_DART_LOGGING")
endif()

if (MEMORY_MODEL_UNIFIED)
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} -DDASH_UNIFIED_MEMORY_MODEL")
endif()

if(MPI_COMPILE_FLAGS)
  set (ADDITIONAL_COMPILE_FLAGS
    "${ADDITIONAL_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS}")
endif()

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_DART_IMPL_MPI_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# Includes
include_directories(
  ${DASH_DART_IMPL_MPI_INCLUDE_DIRS}
  ${MPI_INCLUDE_PATH}
  ${DASH_DART_IF_INCLUDE_DIR}
  ${DASH_DART_BASE_INCLUDE_DIR}
)
# Library compilation sources
add_library(
  ${DASH_DART_IMPL_MPI_LIBRARY} # library name
  ${DASH_DART_IMPL_MPI_SOURCES} # sources
  ${DASH_DART_IMPL_MPI_HEADERS} # headers
)
# Link dependencies
target_link_libraries(
  ${DASH_DART_IMPL_MPI_LIBRARY}
  ${MPI_C_LIBRARIES}
  ${DASH_DART_BASE_LIBRARY}
)

if(MPI_LINK_FLAGS)
  set_target_properties(
    ${DASH_DART_IMPL_MPI_LIBRARY} PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

# Compile flags
set_target_properties(
  ${DASH_DART_IMPL_MPI_LIBRARY} PROPERTIES
  COMPILE_FLAGS ${ADDITIONAL_COMPILE_FLAGS}
)

## Installation

DeployLibrary(${DASH_DART_IMPL_MPI_LIBRARY})

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_DART_IMPL_MPI_LIBRARY} DESTINATION lib)


project(project_dash_dart_impl_mpi)

## User options
set(MPICC_COMPILER "mpic" CACHE STRING  "MPI C compiler wrapper")
set(MPICXX_COMPILER "mpicxx" CACHE STRING  "MPI C++ compiler wrapper")

# Library name
set(DASH_DART_IMPL_MPI_LIBRARY dart-mpi)

include(${CMAKE_SOURCE_DIR}/CMakeExt/MPI.cmake)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_IMPL_MPI_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_IMPL_MPI_HEADERS "include/*.h")

# Include directory to selected version of DART interface
set(DASH_DART_IF_INCLUDE_DIR
   ${CMAKE_CURRENT_SOURCE_DIR}/../../dart-if/v${DART_IF_VERSION}/include)

message("-- MPI include path: " "${MPI_INCLUDE_PATH}")

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_DART_IMPL_MPI_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include_directories(
# SYSTEM ${MPI_INCLUDE_PATH}
  ${DASH_DART_IMPL_MPI_INCLUDE_DIRS}
  ${MPI_INCLUDE_PATH}
  ${DASH_DART_IF_INCLUDE_DIR}
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
)

if(MPI_COMPILE_FLAGS)
  set_target_properties(
    ${DASH_DART_IMPL_MPI_LIBRARY} PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(
    ${DASH_DART_IMPL_MPI_LIBRARY} PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

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


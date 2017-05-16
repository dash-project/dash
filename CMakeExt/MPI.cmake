INCLUDE (CheckSymbolExists)
INCLUDE (CMakePushCheckState)

if (NOT $ENV{MPI_C_COMPILER} STREQUAL "")
  set(MPI_C_COMPILER   $ENV{MPI_C_COMPILER}
      CACHE STRING "MPI C compiler")
endif()
if (NOT $ENV{MPI_CXX_COMPILER} STREQUAL "")
  set(MPI_CXX_COMPILER $ENV{MPI_CXX_COMPILER}
      CACHE STRING "MPI C++ compiler")
endif()

find_package(MPI)

if (MPI_INCLUDE_PATH AND MPI_LIBRARY)
  try_run(RESULT_VAR COMPILE_RESULT_VAR 
          ${CMAKE_BINARY_DIR} 
          ${CMAKE_SOURCE_DIR}/CMakeExt/Code/print_mpi_impl.c 
          CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${MPI_INCLUDE_PATH}
          COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
          RUN_OUTPUT_VARIABLE MPI_STRING)
  if (NOT COMPILE_RESULT_VAR)
    message(FATAL_ERROR "Failed to determine MPI library: ${COMPILE_OUTPUT}")
  endif()
  if (NOT RESULT_VAR EQUAL 0)
    message(FATAL_ERROR "Failed to determine MPI library: ${MPI_STRING}")
  endif()

  string(REPLACE " " ";" MPI_LIST ${MPI_STRING})
  list(GET MPI_LIST 0 MPI_IMPL_NAME)
  list(GET MPI_LIST 1 MPI_IMPL_VERSION)

  message(STATUS "Found MPI implementation: ${MPI_IMPL_NAME} ${MPI_IMPL_VERSION}")

  set(MPI_FOUND TRUE CACHE BOOL "Found the MPI library")
  if (MPI_IMPL_NAME MATCHES "mpich")
    set(MPI_IMPL_IS_MPICH TRUE CACHE BOOL "MPICH detected")
    set(MPI_IMPL_ID "mpich" CACHE STRING "MPI implementation identifier")
  # Cray MPI identifies as MPICH
  elseif ("${MPI_INCLUDE_PATH}" MATCHES "cray")
    set(MPI_IMPL_IS_CRAY TRUE CACHE BOOL "CrayMPI detected")
    set(MPI_IMPL_ID "craympi" CACHE STRING "MPI implementation identifier")
  elseif (MPI_IMPL_NAME MATCHES "mvapich")
    set(MPI_IMPL_IS_MVAPICH TRUE CACHE BOOL "MVAPICH detected")
    set(MPI_IMPL_ID "mvapich" CACHE STRING "MPI implementation identifier")
  elseif (MPI_IMPL_NAME MATCHES "impi")
    set(MPI_IMPL_IS_INTEL TRUE CACHE BOOL "IntelMPI detected")
    set(MPI_IMPL_ID "intelmpi" CACHE STRING "MPI implementation identifier")
  elseif (MPI_IMPL_NAME MATCHES "openmpi")
    set(MPI_IMPL_IS_OPENMPI TRUE CACHE BOOL "OpenMPI detected")
    set(MPI_IMPL_ID "openmpi" CACHE STRING "MPI implementation identifier")
    # Open MPI before 1.10.7 and 2.1.1 failed to properly align memory
    if (${MPI_IMPL_VERSION} VERSION_LESS 1.10.7 OR 
        ${MPI_IMPL_VERSION} VERSION_EQUAL 2.0.0 OR 
        (${MPI_IMPL_VERSION} VERSION_GREATER 2.0.0 AND ${MPI_IMPL_VERSION} VERSION_LESS 2.1.1))
      # disable shared memory windows due to alignment problems
      set(ENABLE_SHARED_WINDOWS OFF)
      message(WARNING 
        "Disabling shared memory window support due to defective allocation "
        "in OpenMPI <2.1.1")
    endif()
  else ()
    message(FATAL_ERROR "Unknown MPI implementation detected: ${MPI_STRING}")
  endif()
else (MPI_INCLUDE_PATH AND MPI_LIBRARY)
  set(MPI_FOUND FALSE CACHE BOOL "Did not find the MPI library")
endif (MPI_INCLUDE_PATH AND MPI_LIBRARY)


# check for MPI-3

# save current state
cmake_push_check_state()
set(CMAKE_REQUIRED_INCLUDES ${MPI_INCLUDE_PATH})
check_symbol_exists(
  MPI_NO_OP
  mpi.h
  HAVE_MPI_NO_OP
)
cmake_pop_check_state()

if (NOT HAVE_MPI_NO_OP)
  message(FATAL_ERROR "Detected MPI library does not support MPI-3.")
endif()


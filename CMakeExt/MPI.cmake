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

if (CMAKE_CROSSCOMPILE)
  message(STATUS "Cross compiling: ${CMAKE_CROSSCOMPILE}")
endif()

find_package(MPI)


# save current state
cmake_push_check_state()
set (CMAKE_REQUIRED_INCLUDES ${MPI_INCLUDE_PATH})

# check for Open MPI
check_symbol_exists(
  OMPI_MAJOR_VERSION
  mpi.h
  HAVE_OPEN_MPI
)
if (HAVE_OPEN_MPI)
  set (MPI_IMPL_IS_OPENMPI TRUE CACHE BOOL "OpenMPI detected")
  set (MPI_IMPL_ID "openmpi" CACHE STRING "MPI implementation identifier")
  try_compile(OMPI_OK ${CMAKE_BINARY_DIR} 
    ${CMAKE_SOURCE_DIR}/CMakeExt/Code/test_compatible_ompi.c 
    CMAKE_FLAGS -DINCLUDE_DIRECTORIES=${MPI_INCLUDE_PATH}
    OUTPUT_VARIABLE OUTPUT)
  if (NOT OMPI_OK)
    message(WARNING 
      "Disabling shared memory window support due to defective allocation "
      "in OpenMPI <2.1.1")
    set (ENABLE_SHARED_WINDOWS OFF)
  endif()
endif()

# order matters: all of the following
# implementations also define MPICH
if (NOT DEFINED MPI_IMPL_ID)
  # check for Intel MPI
  check_symbol_exists(
    I_MPI_VERSION
    mpi.h
    HAVE_I_MPI
  )
  if (HAVE_I_MPI)
    set (MPI_IMPL_IS_INTEL TRUE CACHE BOOL "IntelMPI detected")
    set (MPI_IMPL_ID "intelmpi" CACHE STRING "MPI implementation identifier")
  endif ()
endif ()

if (NOT DEFINED MPI_IMPL_ID)
  # check for MVAPICH
  check_symbol_exists(
    MVAPICH2_VERSION
    mpi.h
    HAVE_MVAPICH
  )
  if (HAVE_MVAPICH)
    set (MPI_IMPL_IS_MVAPICH TRUE CACHE BOOL "MVAPICH detected")
    set (MPI_IMPL_ID "mvapich" CACHE STRING "MPI implementation identifier")
  endif ()
endif ()

if (NOT DEFINED MPI_IMPL_ID)
  # check for Cray MPI
  # MPIX_PortName_Backlog is a Cray extension
  # TODO: any better way to detect Cray MPI?
  check_symbol_exists(
    MPIX_PortName_Backlog
    mpi.h
    HAVE_CRAY_MPI
  )
  if (HAVE_CRAY_MPI)
    set(MPI_IMPL_IS_CRAY TRUE CACHE BOOL "CrayMPI detected")
    set(MPI_IMPL_ID "craympi" CACHE STRING "MPI implementation identifier")
  endif ()
endif ()

if (NOT DEFINED MPI_IMPL_ID)
  # check for MVAPICH
  check_symbol_exists(
    MPICH
    mpi.h
    HAVE_MPICH
  )
  if (HAVE_MPICH)
    set (MPI_IMPL_IS_MPICH TRUE CACHE BOOL "MPICH detected")
    set (MPI_IMPL_ID "mpich" CACHE STRING "MPI implementation identifier")
  endif ()
endif ()

# restore state
cmake_pop_check_state()

if (NOT DEFINED MPI_IMPL_ID)
  set (MPI_IMPL_ID "UNKNOWN" CACHE STRING "MPI implementation identifier")
endif()

message(STATUS "Detected MPI implementation: ${MPI_IMPL_ID}")

# check for MPI-3
try_compile(HAVE_MPI3 ${CMAKE_BINARY_DIR} 
  ${CMAKE_SOURCE_DIR}/CMakeExt/Code/test_mpi_support.c 
  OUTPUT_VARIABLE OUTPUT)

if (NOT HAVE_MPI3)
  set(MPI_IS_DART_COMPATIBLE FALSE CACHE BOOL
     "MPI LIB has support for MPI-3")
  unset (MPI_IMPL_ID CACHE)
  message (WARNING 
    "Detected MPI implementation (${MPI_IMPL_ID}) does not support MPI-3")
else()
  set(MPI_IS_DART_COMPATIBLE TRUE CACHE BOOL
    "MPI LIB has support for MPI-3")
endif()


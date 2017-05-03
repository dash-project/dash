
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
  set(MPI_FOUND TRUE CACHE BOOL "Found the MPI library")
  if ("${MPI_INCLUDE_PATH}" MATCHES "mpich")
    set(MPI_IMPL_IS_MPICH TRUE CACHE BOOL "MPICH detected")
    set(MPI_IMPL_ID "mpich" CACHE STRING "MPI implementation identifier")
  elseif ("${MPI_INCLUDE_PATH}" MATCHES "cray")
    set(MPI_IMPL_IS_CRAY TRUE CACHE BOOL "CrayMPI detected")
    set(MPI_IMPL_ID "craympi" CACHE STRING "MPI implementation identifier")
  elseif ("${MPI_INCLUDE_PATH}" MATCHES "mvapich")
    set(MPI_IMPL_IS_MVAPICH TRUE CACHE BOOL "MVAPICH detected")
    set(MPI_IMPL_ID "mvapich" CACHE STRING "MPI implementation identifier")
  elseif ("${MPI_INCLUDE_PATH}" MATCHES "impi"
          OR "${MPI_INCLUDE_PATH}" MATCHES "intel")
    set(MPI_IMPL_IS_INTEL TRUE CACHE BOOL "IntelMPI detected")
    set(MPI_IMPL_ID "intelmpi" CACHE STRING "MPI implementation identifier")
  elseif ("${MPI_INCLUDE_PATH}" MATCHES "openmpi")
    set(MPI_IMPL_IS_OPENMPI TRUE CACHE BOOL "OpenMPI detected")
    set(MPI_IMPL_ID "openmpi" CACHE STRING "MPI implementation identifier")
    # temporarily disable shared memory windows due to alignment problems
    set(ENABLE_SHARED_WINDOWS OFF)
    message(WARNING "MPI shared windows disabled due to defective allocation in OpenMPI")
  endif()
else (MPI_INCLUDE_PATH AND MPI_LIBRARY)
  set(MPI_FOUND FALSE CACHE BOOL "Did not find the MPI library")
endif (MPI_INCLUDE_PATH AND MPI_LIBRARY)

# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which icc`
#   export CXX=`which icc`
#
set(ENV{CXX} mpiicpc)
set(ENV{CC} mpiicc)

set(DASH_ENV_HOST_SYSTEM_ID "supermuc" CACHE STRING
    "Host system type identifier")

# Disable MPI shared windows for IBM MPI on SuperMUC:
if ("$ENV{MP_MPILIB}" STREQUAL "mpich2")
  message(NOTE "MPI shared windows are disabled for IBM MPI on SuperMUC")
  set (ENABLE_SHARED_WINDOWS OFF CACHE BOOL
       "MPI shared windows are disabled for IBM MPI on SuperMUC" FORCE)
  option(ENABLE_SHARED_WINDOWS OFF)
endif()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopenmp -xhost -mkl")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mt_mpi")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")
endif()
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopenmp -xhost -mkl")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mt_mpi")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")
endif()

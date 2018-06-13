# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which mpiicc`
#   export CXX=`which mpiicc`
#
set(ENV{CXX} mpiicpc)
set(ENV{CC} mpiicc)

set(DASH_ENV_HOST_SYSTEM_ID "supermic" CACHE STRING
    "Host system type identifier")

# Force Intel MPI implementation:
if (NOT "$ENV{MPI_BASE}" MATCHES "intel")
  message(ERROR "MIC build requires Intel MPI")
endif()

set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopenmp -mmic -mkl")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mt_mpi")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -Wl,-rpath,$ENV{MIC_LD_LIBRARY_PATH}")

set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopenmp -mmic -mkl")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mt_mpi")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -Wl,-rpath,$ENV{MIC_LD_LIBRARY_PATH}")

set(MKLROOT $ENV{MKLROOT})

# Sequential MKL
# set(MKL_LINK_FLAGS
#     ${MKLROOT}/lib/mic/libmkl_scalapack_lp64.a
#     -Wl,--start-group
#     ${MKLROOT}/lib/mic/libmkl_intel_lp64.a
#     ${MKLROOT}/lib/mic/libmkl_core.a
#     ${MKLROOT}/lib/mic/libmkl_sequential.a
#     ${MKLROOT}/lib/mic/libmkl_blacs_intelmpi_lp64.a
#     -Wl,--end-group
#     -lpthread -lm -ldl)

# Multithreaded MKL
set(MKL_LINK_FLAGS
    ${MKLROOT}/lib/mic/libmkl_scalapack_lp64.a
    -Wl,--start-group
    ${MKLROOT}/lib/mic/libmkl_intel_lp64.a
    ${MKLROOT}/lib/mic/libmkl_core.a
    ${MKLROOT}/lib/mic/libmkl_intel_thread.a
    ${MKLROOT}/lib/mic/libmkl_blacs_intelmpi_lp64.a
    -Wl,--end-group
    -lpthread -lm -ldl)

set(MKL_INCLUDE_DIRS "${MKLROOT}/include")
set(MKL_LIBRARIES "")

set(MKL_FOUND TRUE)

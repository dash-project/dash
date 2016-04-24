# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which mpiicc`
#   export CXX=`which mpiicc`
#
set(ENV{CXX} mpiicc)
set(ENV{CC} mpiicc)

set(DASH_ENV_HOST_SYSTEM_ID "supermic" CACHE STRING
    "Host system type identifier")

set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mmic")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mmic")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

set(MKLROOT $ENV{MKLROOT})

set(MKL_LIBRARIES "${MKL_LIBRARIES} ${MKLROOT}/lib/mic/libmkl_scalapack_lp64.a")
set(MKL_LIBRARIES "${MKL_LIBRARIES} -Wl,--start-group")
set(MKL_LIBRARIES "${MKL_LIBRARIES} ${MKLROOT}/lib/mic/libmkl_intel_lp64.a")
set(MKL_LIBRARIES "${MKL_LIBRARIES} ${MKLROOT}/lib/mic/libmkl_core.a")
set(MKL_LIBRARIES "${MKL_LIBRARIES} ${MKLROOT}/lib/mic/libmkl_sequential.a")
set(MKL_LIBRARIES "${MKL_LIBRARIES} ${MKLROOT}/lib/mic/libmkl_blacs_intelmpi_lp64.a")
set(MKL_LIBRARIES "${MKL_LIBRARIES} -Wl,--end-group -lpthread -lm -ldl")

set(MKL_INCLUDE_DIRS "${MKLROOT}/include")

set(MKL_FOUND TRUE)

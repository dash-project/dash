# Configuration and CMake modules for SuperMIC

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

set(DASH_ENV_HOST_MANYCORE TRUE CACHE STRING
    "Whether the system is host of an Intel(R) MIC (Xeon Phi) architecture")

set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

set(CC_ENV_SETUP_MIC_FLAGS  "${CC_ENV_SETUP_FLAGS} -mmic")
set(CXX_ENV_SETUP_MIC_FLAGS "${CXX_ENV_SETUP_FLAGS} -mmic")


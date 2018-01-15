# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which mpiicc`
#   export CXX=`which mpiicc`
#

set(ENV{CXX} cc)
set(ENV{CC} CC)

set(DASH_ENV_HOST_SYSTEM_ID "cori knl" CACHE STRING
    "Host system type identifier")

#set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -xMIC-AVX512 -qopenmp")
#set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mt_mpi")
#set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
#set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

#set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -xMIC-AVX512 -qopenmp")
#set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mt_mpi")
#set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
#set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

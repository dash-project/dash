# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which icc`
#   export CXX=`which icc`
#
set(ENV{CXX} icc)
set(ENV{CC} icc)

set(DASH_ENV_HOST_SYSTEM_ID "supermuc" CACHE STRING
    "Host system type identifier")

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -xCORE-AVX2")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -xHost")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -opt-streaming-stores always")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -opt-prefetch-distance=64,8")
endif()
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -xCORE-AVX2")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -xHost")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -opt-streaming-stores always")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -opt-prefetch-distance=64,8")
endif()

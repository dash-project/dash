
# Define overrides for options and config flags here

set(DASH_ENV_HOST_SYSTEM_ID "default" CACHE STRING
    "Host system type identifier")

if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mtune=native -march=native")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mfpmath=sse")

  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mtune=native -march=native")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mfpmath=sse")
endif()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopenmp -xhost")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
  set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")

  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopenmp -xhost")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-streaming-stores always")
  set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -qopt-prefetch-distance=64,8")
endif()

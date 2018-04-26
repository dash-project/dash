
# Define overrides for options and config flags here

set(DASH_ENV_HOST_SYSTEM_ID "default" CACHE STRING
    "Host system type identifier")

if (NOT BUILD_GENERIC)
  if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU"
      OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mtune=native")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -march=native")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mfpmath=sse")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -fsanitize=thread")

    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mtune=native")
    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -march=native")
    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mfpmath=sse")
    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -fsanitize=thread")
  endif()

  if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -xhost")
    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -xhost")
    set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -fsanitize=thread")
    set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -fsanitize=thread")
  endif()
endif()

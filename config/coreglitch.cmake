
# Define overrides for options and config flags here

set(DASH_ENV_HOST_SYSTEM_ID "coreglitch" CACHE STRING
    "Host system type identifier")

if (ENABLE_BLAS AND BLAS_FOUND)
  set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
      cblas)
endif()

set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mtune=native -march=native")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -mfpmath=sse")
set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -minline-all-stringops")

set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mtune=native -march=native")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -mfpmath=sse")
set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -minline-all-stringops")

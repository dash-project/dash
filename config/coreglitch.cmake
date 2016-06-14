
# Define overrides for options and config flags here

set(DASH_ENV_HOST_SYSTEM_ID "coreglitch" CACHE STRING
    "Host system type identifier")

if (ENABLE_BLAS AND BLAS_FOUND)
  set(ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
      cblas)
endif()

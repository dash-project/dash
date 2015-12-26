

set(ENVIRONMENT_TYPE "default" CACHE STRING
    "Specify environment setup type. See folder config for available setups")
set(ENVIRONMENT_CONFIG_PATH "" CACHE STRING
    "Specify path to custom environment setup configuration file")

if (NOT "${ENVIRONMENT_TYPE}" STREQUAL "")
  # Environment type has been specified.
  # Load environment config file from ./config.
  set(ENVIRONMENT_CONFIG_PATH
      "${CMAKE_SOURCE_DIR}/config/${ENVIRONMENT_TYPE}.cmake")
  if (NOT EXISTS "${ENVIRONMENT_CONFIG_PATH}")
    message(FATAL_ERROR
            "Unknown environment type: "
            "${ENVIRONMENT_TYPE}")
  endif()
endif()

if (NOT "${ENVIRONMENT_CONFIG_PATH}" STREQUAL "")
  if (NOT EXISTS ${ENVIRONMENT_CONFIG_PATH})
    message(FATAL_ERROR
            "Environment configuration file does not exist: "
            "${ENVIRONMENT_CONFIG_PATH}")
  else()
    message(STATUS
            "Loading environment configuration from "
            "${ENVIRONMENT_CONFIG_PATH}")
    include(${ENVIRONMENT_CONFIG_PATH})
  endif()
endif()


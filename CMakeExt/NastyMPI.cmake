# Try to find NastyMPI
# Warning enable for debugging and testing only
include(ExternalProject)

if(ENABLE_NASTYMPI AND BUILD_TESTS)
  if(NOT "${NASTYMPI_LIBRARY_PATH}" STREQUAL "")

    message(STATUS "Searching for NastyMPI")
    find_library(
      NASTYMPI_LIBRARIES
      NAMES libnasty_mpi.a libnasty_mpi.so
      HINTS ${NASTYMPI_LIBRARY_PATH}
    )

    find_package_handle_standard_args(
      NASTYMPI DEFAULT_MSG
      NASTYMPI_LIBRARIES
    )

    mark_as_advanced(
      NASTYMPI_LIBRARIES
    )

    if (NASTYMPI_FOUND)
      message(STATUS "NastyMPI libraries: " ${NASTYMPI_LIBRARIES})
    endif()
  else()

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(NASTYMPI_BUILD_TARGET "debug")
  else()
    set(NASTYMPI_BUILD_TARGET "release")
  endif()

  # message(INFO "NASTYMPI build command: make ${NASTYMPI_BUILD_TARGET}")

  # Download NastyMPI from official github repository:
  message(STATUS "Downloading NastyMPI")
  set(NASTYMPI_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/nastympi")
  ExternalProject_Add(
    NastyMPIExternal
    GIT_REPOSITORY https://github.com/dash-project/nasty-MPI.git
    GIT_TAG master
    TIMEOUT 10
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/nastympi
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make
    INSTALL_COMMAND make install PREFIX=${NASTYMPI_PREFIX}
    LOG_DOWNLOAD ON
    LOG_BUILD ON
  )
  set(NASTYMPI_LIBRARIES "${NASTYMPI_PREFIX}/src/NastyMPIExternal/lib/libnasty_mpi.so")
  message(INFO "NASTYMPI: ${NASTYMPI_LIBRARIES}")

  set(NASTYMPI_FOUND TRUE)

  endif()
endif()

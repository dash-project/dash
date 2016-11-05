# Try to find NastyMPI
# Warning enable for debugging and testing only

if(NOT "${NASTYMPI_LIBRARY_PATH}" STREQUAL "")
  if(ENABLE_NASTYMPI AND BUILD_TESTS)

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
  endif()
endif()

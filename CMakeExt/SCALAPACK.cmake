
if(NOT SCALAPACK_PREFIX AND NOT $ENV{SCALAPACK_BASE} STREQUAL "")
	set(SCALAPACK_PREFIX $ENV{SCALAPACK_BASE})
  message(STATUS "Searching for ScaLAPACK in path " ${SCALAPACK_PREFIX})
endif()
if(NOT SCALAPACK_PREFIX)
	set(SCALAPACK_PREFIX "/usr/")
endif()

find_library(
	SCALAPACK_LIBRARIES
	NAMES scalapack scalapack-openmpi scalapack-mpich scalapack-mvapich
	HINTS ${SCALAPACK_PREFIX}/lib
)

find_path(
	SCALAPACK_INCLUDE_DIRS
	NAMES scalapack.h
	HINTS ${SCALAPACK_PREFIX}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  SCALAPACK DEFAULT_MSG
  SCALAPACK_LIBRARIES
  SCALAPACK_INCLUDE_DIRS
)

# set flags
if(SCALAPACK_FOUND)
#  set (SCALAPACK_LINKER_FLAGS "-lcblas
endif()

mark_as_advanced(
	SCALAPACK_LIBRARIES
	SCALAPACK_INCLUDE_DIRS
)

if (SCALAPACK_FOUND)
	message(STATUS "SCALAPACK includes:   " ${SCALAPACK_INCLUDE_DIRS})
	message(STATUS "SCALAPACK libraries:  " ${SCALAPACK_LIBRARIES})
endif()

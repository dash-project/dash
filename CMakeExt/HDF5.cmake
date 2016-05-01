# Try to find HDF5 libraries and headers.
# Usage of this module:
#
# find_package(hdf5)
#
# Variables defined by this module:
#
#  HDF5_FOUND
#  HDF5_LIBRARIES
#  HDF5_INCLUDE_DIRS


if(NOT HDF5_PREFIX AND NOT $ENV{HDF5_BASE} STREQUAL "")
	set(HDF5_PREFIX $ENV{HDF5_BASE})
endif()

message(STATUS "Searching for HDF5 library in path " ${HDF5_PREFIX})

message(STATUS "Try to find HDF5 cmake package")
find_package (
	HDF5
	NAMES hdf5 COMPONENTS C
)

if(NOT HDF5_FOUND)
	message(STATUS "HDF5 Package not found, try to find libs")
endif()

find_library(
	HDF5_LIBRARIES
	NAMES hdf5
	HINTS ${HDF5_PREFIX}/lib
)

find_path(
	HDF5_INCLUDE_DIRS
	NAMES hdf5.h
	HINTS ${HDF5_PREFIX}/include
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  HDF5 DEFAULT_MSG
  HDF5_LIBRARIES
  HDF5_INCLUDE_DIRS
)

# set flags
if(HDF5_FOUND)
#  set (HDF5_LINKER_FLAGS "-lhdf5_hl -lhdf5 -ldl -lm -lz")
   set (HDF5_LINKER_FLAGS $ENV{HDF5_LIB} ${SZIP_LIB} -lz)
endif()

mark_as_advanced(
	HDF5_LIBRARIES
	HDF5_INCLUDE_DIRS
)

if (HDF5_FOUND)
	message(STATUS "HDF5 includes:   " ${HDF5_INCLUDE_DIRS})
	message(STATUS "HDF5 libraries:  " ${HDF5_LIBRARIES})
endif()

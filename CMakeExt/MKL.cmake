
# Use FindMKL script if MKL_FOUND has not been set by build configuration yet:
if (NOT MKL_FOUND)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/FindMKL.cmake)
endif()

if (MKL_FOUND)
    if (NOT MKL_FIND_QUIETLY OR MKL_FIND_DEBUG)
        message(STATUS "Intel(R) MKL found:")
        message(STATUS "MKL_INCLUDE_DIRS:        " ${MKL_INCLUDE_DIRS})
        message(STATUS "MKL_LIBRARY_DIRS:        " ${MKL_LIBRARY_DIRS})
        message(STATUS "MKL_LIBRARIES:           " ${MKL_LIBRARIES})
        message(STATUS "MKL_SCALAPACK_LIBRARIES: " ${MKL_SCALAPACK_LIBRARIES})
        message(STATUS "MKL_LINK_FLAGS:")
        foreach (MKL_LINK_FLAG ${MKL_LINK_FLAGS})
          message(STATUS "   " ${MKL_LINK_FLAG})
        endforeach()
    endif()
else()
    if (MKL_FIND_REQUIRED)
        message(SEND_ERROR "Intel(R) MKL could not be found.")
    else()
        message(STATUS "Intel(R) MKL could not be found.")
    endif()
    message(STATUS "(ENV) MKLROOT:       " $ENV{MKLROOT})
    message(STATUS "(ENV) INTELROOT:     " $ENV{INTELROOT})
    message(STATUS "(ENV) SCALAPACK_LIB: " $ENV{SCALAPACK_LIB})
endif()

mark_as_advanced(
    MKL_INCLUDE_DIR
    MKL_INCLUDE_DIRS
    MKL_LIBRARY_DIRS
    MKL_LINK_FLAGS
)


# set (BLA_STATIC ON)
find_package(BLAS)
find_package(LAPACK)

if (LAPACK_FOUND)
  if ("${LAPACK_INCLUDE_DIRS}" STREQUAL "")
    # Temporary workaround
    set(LAPACK_INCLUDE_DIRS "/usr/include/atlas")
  endif()
  message(STATUS "LAPACK includes:  " ${LAPACK_INCLUDE_DIRS})
  message(STATUS "LAPACK libraries: " ${LAPACK_LIBRARIES})
else()
  message(STATUS "LAPACK not found")
endif()


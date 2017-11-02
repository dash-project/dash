
if (${ENABLE_VALGRIND})
  if (NOT ${VALGRIND_PREFIX})
    find_path(
      VALGRIND_PREFIX
      NAMES include/valgrind/valgrind.h
    )
  endif()

  message(STATUS "Searching for Valgrind in path " ${VALGRIND_PREFIX})

  find_path(
    VALGRIND_INCLUDE_DIRS
    NAMES valgrind/valgrind.h
    HINTS ${VALGRIND_PREFIX}/include/
  )

  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(
    VALGRIND DEFAULT_MSG
    VALGRIND_INCLUDE_DIRS
  )

  mark_as_advanced(
    VALGRIND_PREFIX_DIRS
    VALGRIND_INCLUDE_DIRS
  )

  if (NOT VALGRIND_FOUND)
    set(ENABLE_VALGRIND PARENT_SCOPE OFF)
  endif()

endif()

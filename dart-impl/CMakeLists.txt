
set(DART_IMPLEMENTATIONS_LIST ${DART_IMPLEMENTATIONS_LIST}
    PARENT_SCOPE)
set(ENABLE_THREADSUPPORT ${ENABLE_THREADSUPPORT}
    PARENT_SCOPE)


if (ENABLE_THREADSUPPORT)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -DDART_ENABLE_THREADSUPPORT")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -DDART_ENABLE_THREADSUPPORT")
endif ()

# DART requires some additinal CFLAGS
if (DASH_PLATFORM_IS_LINUX OR
    DASH_PLATFORM_IS_OSX   OR
    DASH_PLATFORM_IS_FREEBSD)
  # -D_XOPEN_SOURCE=700 is required for pthread_mutexattr_settype
  # and gethostname
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -D_XOPEN_SOURCE=700")
endif()

# DART base:
add_subdirectory(base)

# DART implementation of every enabled DART variant:
if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";mpi;")
  if(NOT MPI_IS_DART_COMPATIBLE)
    message(FATAL_ERROR "Detected MPI library does not support MPI-3.")
  endif()
  set(DART_IMPLEMENTATION_MPI_ENABLED ON
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(mpi)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";shmem;")
  set(DART_IMPLEMENTATION_SHMEM_ENABLED ON
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(shmem)
endif()

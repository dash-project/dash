INCLUDE (CheckSymbolExists)
INCLUDE (CMakePushCheckState)

if (NOT DEFINED MPI_IMPL_ID)

  if (NOT $ENV{MPI_C_COMPILER} STREQUAL "")
    set(MPI_C_COMPILER   $ENV{MPI_C_COMPILER}
        CACHE STRING "MPI C compiler")
  endif()
  if (NOT $ENV{MPI_CXX_COMPILER} STREQUAL "")
    set(MPI_CXX_COMPILER $ENV{MPI_CXX_COMPILER}
        CACHE STRING "MPI C++ compiler")
  endif()

  # find MPI environment
  find_package(MPI)

  # helper function that executes the MPI compiler
  # on the given source file and sets
  # resvar to true if the compile step succeeded
  function (check_mpi_compile sourcefile resvar)

    # check for MPI-3
    get_filename_component(filename ${sourcefile} NAME)
    execute_process(
      COMMAND "${MPI_C_COMPILER}" -c "${sourcefile}" -o "${CMAKE_BINARY_DIR}/${filename}.o"
      RESULT_VARIABLE RETURN_VAL
      OUTPUT_VARIABLE OUTPUT
      ERROR_VARIABLE  OUTPUT
    )

    if (RETURN_VAL EQUAL 0)
      set (${resvar} TRUE PARENT_SCOPE)
    else ()
      set (${resvar} FALSE PARENT_SCOPE)
      message (STATUS "Failed to execute MPI compiler: \n${OUTPUT}")
    endif ()

  endfunction ()


  # Determine MPI implementation

  # save current state
  cmake_push_check_state()
  set (CMAKE_REQUIRED_INCLUDES ${MPI_INCLUDE_PATH})

  # check for Open MPI
  check_symbol_exists(
    OMPI_MAJOR_VERSION
    mpi.h
    HAVE_OPEN_MPI
  )
  if (HAVE_OPEN_MPI)
    set (MPI_IMPL_IS_OPENMPI TRUE CACHE BOOL "OpenMPI detected")
    set (MPI_IMPL_ID "openmpi" CACHE STRING "MPI implementation identifier")
    set (MPI_CXX_SKIP_FLAGS "-DOMPI_SKIP_MPICXX"
          CACHE STRING "Flag to suppress MPI C++ bindings")
    check_mpi_compile(${CMAKE_SOURCE_DIR}/CMakeExt/Code/test_compatible_ompi.c OMPI_OK)
    if (NOT OMPI_OK)
      message(WARNING
        "Disabling shared memory window support due to defective allocation "
        "in OpenMPI <2.1.1")
      set (ENABLE_SHARED_WINDOWS OFF)
    endif()
  endif()

  # order matters: all of the following
  # implementations also define MPICH
  if (NOT DEFINED MPI_IMPL_ID)
    # check for Intel MPI
    check_symbol_exists(
      I_MPI_VERSION
      mpi.h
      HAVE_I_MPI
    )
    if (HAVE_I_MPI)
      set (MPI_IMPL_IS_INTEL TRUE CACHE BOOL "IntelMPI detected")
      set (MPI_IMPL_ID "intelmpi" CACHE STRING "MPI implementation identifier")
      set (MPI_CXX_SKIP_FLAGS "-DOMPI_SKIP_MPICXX"
            CACHE STRING "Flag to suppress MPI C++ bindings")
    endif ()
  endif ()

  if (NOT DEFINED MPI_IMPL_ID)
    # check for MVAPICH
    check_symbol_exists(
      MVAPICH2_VERSION
      mpi.h
      HAVE_MVAPICH
    )
    if (HAVE_MVAPICH)
      set (MPI_IMPL_IS_MVAPICH TRUE CACHE BOOL "MVAPICH detected")
      set (MPI_IMPL_ID "mvapich" CACHE STRING "MPI implementation identifier")
      set (MPI_CXX_SKIP_FLAGS "-DMPICH_SKIP_MPICXX"
            CACHE STRING "Flag to suppress MPI C++ bindings")
    endif ()
  endif ()

  if (NOT DEFINED MPI_IMPL_ID)
    # check for Cray MPI
    check_symbol_exists(
      CRAY_MPICH_VERSION
      mpi.h
      HAVE_CRAY_MPI
    )
    if (NOT HAVE_CRAY_MPI)
      # fall-back for versions prior to MPT 7.6.0
      # MPIX_PortName_Backlog is a Cray extension
      check_symbol_exists(
        MPIX_PortName_Backlog
        mpi.h
        HAVE_CRAY_MPI
      )
    endif()
    if (HAVE_CRAY_MPI)
      set(MPI_IMPL_IS_CRAY TRUE CACHE BOOL "CrayMPI detected")
      set(MPI_IMPL_ID "craympi" CACHE STRING "MPI implementation identifier")
      set (MPI_CXX_SKIP_FLAGS "-DMPICH_SKIP_MPICXX"
            CACHE STRING "Flag to suppress MPI C++ bindings")
    endif ()
  endif ()

  if (NOT DEFINED MPI_IMPL_ID)
    # check for MVAPICH
    check_symbol_exists(
      MPICH
      mpi.h
      HAVE_MPICH
    )
    if (HAVE_MPICH)
      set (MPI_IMPL_IS_MPICH TRUE CACHE BOOL "MPICH detected")
      set (MPI_IMPL_ID "mpich" CACHE STRING "MPI implementation identifier")
      set (MPI_CXX_SKIP_FLAGS "-DMPICH_SKIP_MPICXX"
            CACHE STRING "Flag to suppress MPI C++ bindings")
    endif ()
  endif ()

  # restore state
  cmake_pop_check_state()

  if (NOT DEFINED MPI_IMPL_ID)
    set (MPI_IMPL_ID "UNKNOWN" CACHE STRING "MPI implementation identifier")
  endif()

  message(STATUS "Detected MPI implementation: ${MPI_IMPL_ID}")
  message(STATUS "Detected MPI C compiler: ${MPI_C_COMPILER}")

  check_mpi_compile(${CMAKE_SOURCE_DIR}/CMakeExt/Code/test_mpi_support.c HAVE_MPI3)

  if (NOT HAVE_MPI3)
    message(${OUTPUT})
    set(MPI_IS_DART_COMPATIBLE FALSE CACHE BOOL
      "MPI LIB has support for MPI-3")
    message (WARNING
      "Detected MPI implementation (${MPI_IMPL_ID}) does not support MPI-3")
    message (STATUS "${OUTPUT}")
    unset (MPI_IMPL_ID CACHE)
  else()
    set(MPI_IS_DART_COMPATIBLE TRUE CACHE BOOL
      "MPI LIB has support for MPI-3")
  endif()

  set (CMAKE_C_COMPILER ${CMAKE_C_COMPILER_SAFE})
  set (MPI_INCLUDE_PATH ${MPI_INCLUDE_PATH}   CACHE STRING "MPI include path")
  set (MPI_C_LIBRARIES  ${MPI_C_LIBRARIES}    CACHE STRING "MPI C libraries")
  set (MPI_LINK_FLAGS   ${MPI_LINK_FLAGS}     CACHE STRING "MPI link flags")
  set (MPI_CXX_FLAGS    ${MPI_CXX_SKIP_FLAGS} CACHE STRING "MPI C++ compile flags")

  message(STATUS "Detected MPI library: ${MPI_IMPL_ID}")
else (NOT DEFINED MPI_IMPL_ID)

  message(STATUS "Using previously detected MPI library: ${MPI_IMPL_ID}")

endif(NOT DEFINED MPI_IMPL_ID)

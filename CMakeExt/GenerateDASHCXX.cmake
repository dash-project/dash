# Generate and install DASH compiler wrapper

# prepare additional compile flags variable
set(ADDITIONAL_COMPILE_FLAGS_WRAP "")
foreach (ADDITIONAL_COMPILE_FLAG ${DASH_ADDITIONAL_COMPILE_FLAGS})
  set(ADDITIONAL_COMPILE_FLAGS_WRAP "${ADDITIONAL_COMPILE_FLAGS_WRAP} ${ADDITIONAL_COMPILE_FLAG}")
endforeach()

# prepare additional includes variable
set(ADDITIONAL_INCLUDES_WRAP "")
foreach (ADDITIONAL_INCLUDE ${DASH_ADDITIONAL_INCLUDES})
  set(ADDITIONAL_INCLUDES_WRAP "${ADDITIONAL_INCLUDES_WRAP} -I${ADDITIONAL_INCLUDE}")
endforeach()


# prepare additional libraries variable
set(ADDITIONAL_LIBRARIES_WRAP "")
foreach (ADDITIONAL_LIB ${DASH_ADDITIONAL_LIBRARIES})
  if (${ADDITIONAL_LIB} MATCHES "^/.*$")
    set(ADDITIONAL_LIBRARIES_WRAP
      "${ADDITIONAL_LIBRARIES_WRAP} ${ADDITIONAL_LIB}")
  else()
    set(ADDITIONAL_LIBRARIES_WRAP
      "${ADDITIONAL_LIBRARIES_WRAP} -l${ADDITIONAL_LIB}")
  endif()
endforeach()

set(CXXFLAGS_WRAP "${CXXFLAGS}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CXXFLAGS_WRAP "${CXXFLAGS_WRAP} ${CMAKE_CXX_FLAGS_DEBUG}")
else ()
  set(CXXFLAGS_WRAP "${CXXFLAGS_WRAP} ${CMAKE_CXX_FLAGS_RELEASE}")
endif()


if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";mpi;")
  foreach (MPI_C_LIB ${MPI_C_LIBRARIES})
    set(ADDITIONAL_LIBRARIES_WRAP
        "${ADDITIONAL_LIBRARIES_WRAP} ${MPI_C_LIB}")
  endforeach()
  set(DASHCC ${CMAKE_CXX_COMPILER})
  set(DART_IMPLEMENTATION "mpi")
  configure_file(
    ${CMAKE_SOURCE_DIR}/dash/scripts/dashcc/dashcxx.in
    ${CMAKE_BINARY_DIR}/bin/dash-mpicxx
    @ONLY)

  install(
    FILES ${CMAKE_BINARY_DIR}/bin/dash-mpicxx
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
                OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)

  install(CODE "execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink dash-mpicxx dash-mpiCC
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
    )"
  )

  install(CODE "execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink dash-mpicxx dash-mpic++
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
    )"
  )
endif()


if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";shmem;")
  set(DASHCC ${CMAKE_CXX_COMPILER})
  set(DART_IMPLEMENTATION "shmem")
  configure_file(
    ${CMAKE_SOURCE_DIR}/dash/scripts/dashcc/dashcxx.in
    ${CMAKE_BINARY_DIR}/bin/dash-shmemcxx
    @ONLY)

  install(
    FILES ${CMAKE_BINARY_DIR}/bin/dash-shmemcxx
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
                OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)

  install(CODE "execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink dash-shmemcxx dash-shmemCC
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
    )"
  )

  install(CODE "execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink dash-shmemcxx dash-shmemc++
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
    )"
  )
endif()

project(project_dash_libdash CXX)

# Load global build settings
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR}
    PARENT_SCOPE)
set(DART_IMPLEMENTATIONS_LIST ${DART_IMPLEMENTATIONS_LIST}
    PARENT_SCOPE)
set(BUILD_EXAMPLES ${BUILD_EXAMPLES}
    PARENT_SCOPE)
set(BUILD_TESTS ${BUILD_TESTS}
    PARENT_SCOPE)
set(BUILD_COVERAGE_TESTS ${BUILD_COVERAGE_TESTS}
    PARENT_SCOPE)
set(ENABLE_LOGGING ${ENABLE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_TEST_LOGGING ${ENABLE_TEST_LOGGING}
    PARENT_SCOPE)
set(ENABLE_TRACE_LOGGING ${ENABLE_TRACE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_THREADSUPPORT ${ENABLE_THREADSUPPORT}
    PARENT_SCOPE)
set(ENABLE_DEFAULT_INDEX_TYPE_LONG ${ENABLE_DEFAULT_INDEX_TYPE_LONG}
    PARENT_SCOPE)
set(ENABLE_SHARED_WINDOWS ${ENABLE_SHARED_WINDOWS}
    PARENT_SCOPE)
set(ENABLE_SCALAPACK ${ENABLE_SCALAPACK}
    PARENT_SCOPE)
set(ENABLE_BLAS ${ENABLE_BLAS}
    PARENT_SCOPE)
set(ENABLE_LIBNUMA ${ENABLE_LIBNUMA}
    PARENT_SCOPE)
set(ENABLE_HWLOC ${ENABLE_HWLOC}
    PARENT_SCOPE)
set(ENABLE_HDF5 ${ENABLE_HDF5}
    PARENT_SCOPE)
set(ENABLE_LIKWID ${ENABLE_LIKWID}
    PARENT_SCOPE)
set(ENABLE_PAPI ${ENABLE_PAPI}
    PARENT_SCOPE)
set(ENABLE_COMPTIME_RED ${ENABLE_COMPTIME_RED}
    PARENT_SCOPE)
set(ENABLE_MEMKIND ${ENABLE_MEMKIND}
    PARENT_SCOPE)


# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_LIBRARY_SOURCES
     "src/*.c" "src/*.h" "src/*.cc" "src/*.cpp" )
file(GLOB_RECURSE DASH_LIBRARY_HEADERS
     "include/*.h")

file(GLOB_RECURSE DASH_TEST_SOURCES
  "test/TestBase.h" "test/TestPrinter.h" "test/main.cc")

foreach(TESTCASE ${TESTCASES_LIST})
file(GLOB_RECURSE DASH_TEST_SOURCES
  ${DASH_TEST_SOURCES} "test/${TESTCASE}.h" "test/${TESTCASE}.cc")
endforeach()

# Directories containing the implementation of the library (-I):
set(DASH_LIBRARY_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
include_directories(
  ${DASH_LIBRARY_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
)

# generate version header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dash/Version.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dash/Version.h)

set (ADDITIONAL_COMPILE_FLAGS ${ADDITIONAL_COMPILE_FLAGS} -DDASH)

if (ENABLE_DEFAULT_INDEX_TYPE_LONG)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_DEFAULT_INDEX_TYPE_LONG)
endif()
if (ENABLE_THREADSUPPORT)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_THREADSUPPORT)
endif()
if (ENABLE_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LOGGING)
endif()
if (ENABLE_TEST_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_TEST_LOGGING)
endif()
if (ENABLE_TRACE_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_TRACE_LOGGING)
endif()
if (PAPI_FOUND AND ENABLE_PAPI)
  set (CONF_AVAIL_PAPI "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_PAPI)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${PAPI_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${PAPI_LIBRARIES})
elseif(NOT APPLE)
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       rt)
endif()
if (HWLOC_FOUND AND ENABLE_HWLOC)
  set (CONF_AVAIL_HWLOC "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_HWLOC)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${HWLOC_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${HWLOC_LIBRARIES})
endif()
if (LIKWID_FOUND AND ENABLE_LIKWID)
  set (CONF_AVAIL_LIKWID "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LIKWID)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${LIKWID_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${LIKWID_LIBRARIES})
endif()
if (NUMA_FOUND AND ENABLE_LIBNUMA)
  set (CONF_AVAIL_NUMA "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_NUMA)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${NUMA_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${NUMA_LIBRARIES})
endif()

if (ENABLE_PLASMA AND PLASMA_FOUND)
  set (CONF_AVAIL_PLASMA "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_PLASMA)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${PLASMA_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${PLASMA_LIBRARIES})
endif()

if (ENABLE_HDF5 AND HDF5_FOUND)
  set (CONF_AVAIL_HDF5 "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_HDF5)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${HDF5_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${HDF5_LIBRARIES})
	set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
       ${HDF5_LINKER_FLAGS})
endif()

if (ENABLE_MEMKIND AND MEMKIND_FOUND)
  set (CONF_AVAIL_MEMKIND "true")
  set (ADDITIONAL_COMPILE_FLAGS
    ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_MEMKIND)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
    ${MEMKIND_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
    ${MEMKIND_LIBRARIES})
        set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
    ${MEMKIND_LINKER_FLAGS})
endif()


if (ENABLE_MKL AND MKL_FOUND)
  message (STATUS "    Intel MKL enabled")
  set (CONF_AVAIL_MKL "true")
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_MKL)
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${MKL_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${MKL_LIBRARIES})
  if (MKL_LINK_FLAGS)
    set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
         ${MKL_LINK_FLAGS})
  endif()
else()
  if (ENABLE_BLAS AND BLAS_FOUND)
    message (STATUS "    BLAS enabled")
    set (CONF_AVAIL_BLAS "true")
    set (ADDITIONAL_COMPILE_FLAGS
         ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_BLAS)
    set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
         ${BLAS_INCLUDE_DIRS})
    set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${BLAS_LIBRARIES})
    set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${BLAS95_LIBRARIES})
    set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
         ${BLAS_LINKER_FLAGS})
  endif()

  if (ENABLE_LAPACK AND LAPACK_FOUND)
    message (STATUS "    LAPACK enabled")
    set (CONF_AVAIL_LAPACK "true")
    set (ADDITIONAL_COMPILE_FLAGS
         ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LAPACK)
    set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
         ${LAPACK_INCLUDE_DIRS})
    set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${LAPACK_LIBRARIES})
  endif()
endif()

if (ENABLE_SCALAPACK)
  if (MKL_SCALAPACK_FOUND)
    set (CONF_AVAIL_SCALAPACK "true")
    set (ADDITIONAL_COMPILE_FLAGS
         ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_SCALAPACK)
    set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${MKL_SCALAPACK_LIBRARIES})
  elseif(SCALAPACK_FOUND)
    set (CONF_AVAIL_SCALAPACK "true")
    set (ADDITIONAL_COMPILE_FLAGS
         ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_SCALAPACK)
    set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${SCALAPACK_LIBRARIES})
  endif()
endif()

if (HAVE_STD_TRIVIALLY_COPYABLE)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_HAVE_STD_TRIVIALLY_COPYABLE)
endif()
if (HAVE_TRIVIAL_COPY_INTRINSIC)
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_HAVE_TRIVIAL_COPY_INTRINSIC)
endif()

# enable algorithms which are supported by current build config
if (CONF_AVAIL_MKL OR CONF_AVAIL_BLAS OR (CMAKE_BUILD_TYPE MATCHES DEBUG))
  message (STATUS "    SUMMA algorithm enabled")
  set(CONF_AVAIL_ALGO_SUMMA "true")
endif()

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  set (ADDITIONAL_COMPILE_FLAGS
    ${ADDITIONAL_COMPILE_FLAGS} -DDASH_DEBUG)
endif()

if (DASH_ENV_HOST_SYSTEM_ID)
  set (HOST_ID ${DASH_ENV_HOST_SYSTEM_ID})
  set (ADDITIONAL_COMPILE_FLAGS
       ${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENV_HOST_SYSTEM_ID='${HOST_ID}')
endif()

set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
     pthread)

message (STATUS "DASH additional compile flags:")
set(ADDITIONAL_COMPILE_FLAGS_STR "")
foreach (ADDITIONAL_FLAG ${ADDITIONAL_COMPILE_FLAGS})
  message (STATUS "    " ${ADDITIONAL_FLAG})
  set(ADDITIONAL_COMPILE_FLAGS_STR
      "${ADDITIONAL_COMPILE_FLAGS_STR} ${ADDITIONAL_FLAG}")
endforeach()
message (STATUS "DASH additional libraries:")
foreach (ADDITIONAL_LIB ${ADDITIONAL_LIBRARIES})
  message (STATUS "    " ${ADDITIONAL_LIB})
endforeach()

set(DASH_ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES} PARENT_SCOPE)
set(DASH_ADDITIONAL_INCLUDES  ${ADDITIONAL_INCLUDES} PARENT_SCOPE)
set(DASH_ADDITIONAL_COMPILE_FLAGS  ${ADDITIONAL_COMPILE_FLAGS} PARENT_SCOPE)

## Build targets

# DASH library for every enabled DART variant:
#
foreach (dart_variant ${DART_IMPLEMENTATIONS_LIST})
  string(TOUPPER ${dart_variant} DART_VARIANT)

  set (DART_LIBRARY "dart-${dart_variant}")
  set (DASH_LIBRARY "dash-${dart_variant}")
  set (VARIANT_ADDITIONAL_COMPILE_FLAGS "")
  if (${dart_variant} STREQUAL "mpi")
    if (IPM_FOUND)
      set (VARIANT_ADDITIONAL_COMPILE_FLAGS
           "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_IPM")
    endif()
    if (NOT ENABLE_SHARED_WINDOWS)
      set (VARIANT_ADDITIONAL_COMPILE_FLAGS
           "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -DDART_MPI_DISABLE_SHARED_WINDOWS")
    endif()
    set (VARIANT_ADDITIONAL_COMPILE_FLAGS
         "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -DDASH_MPI_IMPL_ID='${MPI_IMPL_ID}'")
    set (VARIANT_ADDITIONAL_COMPILE_FLAGS
         "${VARIANT_ADDITIONAL_COMPILE_FLAGS} ${MPI_CXX_FLAGS}")
  endif()
  set(${DART_VARIANT}_ADDITIONAL_COMPILE_FLAGS ${VARIANT_ADDITIONAL_COMPILE_FLAGS} PARENT_SCOPE)
  set (VARIANT_ADDITIONAL_COMPILE_FLAGS
         "${VARIANT_ADDITIONAL_COMPILE_FLAGS} ${ADDITIONAL_COMPILE_FLAGS_STR}")
  set (VARIANT_ADDITIONAL_COMPILE_FLAGS
       "${VARIANT_ADDITIONAL_COMPILE_FLAGS} ${CXX_OMP_FLAG}")
  message(STATUS "Building DASH library    " ${DASH_LIBRARY})

  # generate dash StaticConfig.h
  configure_file("include/dash/util/StaticConfig.h.in"
                 "${CMAKE_SOURCE_DIR}/dash/include/dash/util/StaticConfig.h")

  # Library compilation sources
  add_library(
    ${DASH_LIBRARY}         # library name
    ${DASH_LIBRARY_SOURCES} # sources
    ${DASH_LIBRARY_HEADERS} # headers
  )
  target_link_libraries(
    ${DASH_LIBRARY}           # library name
    ${DASH_LINK_DART_LIBRARY} # dependency: DART lib
    ${ADDITIONAL_LINK_FLAGS}
    ${ADDITIONAL_LIBRARIES}
  )

  set_target_properties(
    ${DASH_LIBRARY}
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE
  )

  if (${dart_variant} STREQUAL "mpi")
    include_directories(
      ${MPI_INCLUDE_PATH})
    target_link_libraries(
      ${DASH_LIBRARY}
      ${MPI_C_LIBRARIES}
      ${MPI_CXX_LIBRARIES})
  endif()
  set_target_properties(
    ${DASH_LIBRARY} PROPERTIES
    COMPILE_FLAGS "${VARIANT_ADDITIONAL_COMPILE_FLAGS}"
  )
  set_target_properties(
    ${DASH_LIBRARY} PROPERTIES
    CXX_STANDARD ${DASH_CXX_STD_PREFERED}
    CXX_STANDARD_REQUIRED ON
  )

  if(ENABLE_COMPTIME_RED)
      #cotire(${DASH_LIBRARY})
  endif()

  include_directories(
    ${ADDITIONAL_INCLUDES}
  )

  DeployLibrary(${DASH_LIBRARY})

  # cmake packaging
  if(${CMAKE_VERSION} VERSION_GREATER 3.0.0 )
	  include(CMakePackageConfigHelpers)

	  target_include_directories("${DASH_LIBRARY}"
	                              PUBLIC $<INSTALL_INTERFACE:include>
	                              PUBLIC ${ADDITIONAL_INCLUDES})
  endif()

  # Install library
  install(TARGETS ${DASH_LIBRARY}
          DESTINATION lib
          EXPORT "${DASH_LIBRARY}-targets")

  # Exports
  install(
    EXPORT "${DASH_LIBRARY}-targets"
    DESTINATION share/cmake)

	if(${CMAKE_VERSION} VERSION_GREATER 3.0.0 )
  	configure_package_config_file(
      "dash-config.cmake.in"
      "${DASH_LIBRARY}-config.cmake"
      INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake")

  	# install custom config
  	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${DASH_LIBRARY}-config.cmake"
            DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake")
	endif()

  # Examples
  if (BUILD_EXAMPLES)
    SubDirList(
      DASH_TEST_DIRECTORIES
      ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    foreach(examplename ${DASH_TEST_DIRECTORIES})
      set(exampletarget ${examplename}.${dart_variant})
      if (EXISTS
         ${CMAKE_CURRENT_SOURCE_DIR}/examples/${examplename}/main.cpp)

        # Build example for single DART implementation
        add_executable(
          ${exampletarget}
          examples/${examplename}/main.cpp
        )
        include_directories(
          ${CMAKE_SOURCE_DIR}/dash/include
          ${ADDITIONAL_INCLUDES}
        )
        target_link_libraries(
          ${exampletarget}
          ${DASH_LIBRARY}
          ${DART_LIBRARY}
          ${ADDITIONAL_LINK_FLAGS}
          ${ADDITIONAL_LIBRARIES}
        )
        if (${dart_variant} STREQUAL "mpi")
          include_directories(
            ${MPI_INCLUDE_PATH})
          target_link_libraries(
            ${exampletarget}
            ${MPI_C_LIBRARIES}
            ${MPI_CXX_LIBRARIES})
          if (IPM_FOUND)
            include_directories(
              ${IPM_INCLUDE_DIRS})
            target_link_libraries(
              ${exampletarget}
              ${IPM_LIBRARIES})
          endif()
          if (PLASMA_FOUND)
            target_link_libraries(
              ${exampletarget}
              ${PLASMA_LIBRARIES})
          endif()
        endif()
        set_target_properties(
          ${exampletarget} PROPERTIES
          COMPILE_FLAGS
          "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -Wno-unused"
        )
        set_target_properties(
          ${exampletarget} PROPERTIES
          CXX_STANDARD ${DASH_CXX_STD_PREFERED}
          CXX_STANDARD_REQUIRED ON
        )
      endif()
    endforeach(examplename ${DASH_TEST_DIRECTORIES})
  endif()
endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})

## Tests
#
if (BUILD_TESTS)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/GoogleTest.cmake)

  if (GTEST_FOUND)
    if (BUILD_COVERAGE_TESTS)
        #include(${CMAKE_SOURCE_DIR}/CMakeExt/CodeCoverage.cmake)
    endif()

    foreach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
      set(DASH_LIBRARY "dash-${dart_variant}")
      set(DART_LIBRARY "dart-${dart_variant}")
      set(DASH_TEST "dash-test-${dart_variant}")
      include_directories(
        ${GTEST_INCLUDES}
        ${CMAKE_SOURCE_DIR}/dash/include
        ${ADDITIONAL_INCLUDES}
      )
      add_executable(
        ${DASH_TEST}
        ${DASH_TEST_SOURCES}
      )
      target_link_libraries(
        ${DASH_TEST}
        ${GTEST_LIBRARY}
        ${DASH_LIBRARY}
        ${DART_LIBRARY}
        ${ADDITIONAL_LIBRARIES}
      )
      if (${dart_variant} STREQUAL "mpi")
        if (IPM_FOUND)
          include_directories(
            ${IPM_INCLUDE_DIRS})
          target_link_libraries(
            ${exampletarget}
            ${IPM_LIBRARIES})
        endif()
      endif()
      set_target_properties(
        ${DASH_TEST} PROPERTIES
        COMPILE_FLAGS
        "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -Wno-unused -Wno-sign-compare"
      )
      set_target_properties(
        ${DASH_TEST} PROPERTIES
        CXX_STANDARD ${DASH_CXX_STD_PREFERED}
        CXX_STANDARD_REQUIRED ON
      )

      if(ENABLE_COMPTIME_RED)
          #cotire(${DASH_TEST})
      endif()

      if(INSTALL_TESTS)
        DeployBinary(${DASH_TEST})
        install(
          TARGETS ${DASH_TEST}
          DESTINATION bin/)
      endif()

      # Code Coverage
      if (BUILD_COVERAGE_TESTS)
        # exclude stdlib
        set(COVERAGE_EXCLUDES '/usr/*')
        separate_arguments(EXEC_FLAGS UNIX_COMMAND $ENV{MPI_EXEC_FLAGS})
        if (${dart_variant} STREQUAL "mpi")
          setup_target_for_coverage_cobertura(
            NAME coverage
            EXECUTABLE mpirun -n 4 ${EXEC_FLAGS} ${CMAKE_BINARY_DIR}/dash/${DASH_TEST}
            DEPENDENCIES coverage)
        else()
          setup_target_for_coverage_cobertura(
            NAME coverage
            EXECUTABLE "${CMAKE_BINARY_DIR}/dash/${DASH_TEST}"
            DEPENDENCIES coverage)
        endif()
      endif()

    endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
  endif(GTEST_FOUND)
endif()

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/cpp17
        DESTINATION include FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/libdash.h
        DESTINATION include)

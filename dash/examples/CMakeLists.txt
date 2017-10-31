project(project_dash_examples CXX)

# Load global build settings
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR}
    PARENT_SCOPE)
set(DART_IMPLEMENTATIONS_LIST ${DART_IMPLEMENTATIONS_LIST}
    PARENT_SCOPE)
set(BUILD_EXAMPLES ${BUILD_EXAMPLES}
    PARENT_SCOPE)
set(BUILD_TESTS ${BUILD_TESTS}
    PARENT_SCOPE)
set(ENABLE_LOGGING ${ENABLE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_TRACE_LOGGING ${ENABLE_TRACE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_UNIFIED_MEMORY_MODEL ${ENABLE_UNIFIED_MEMORY_MODEL}
    PARENT_SCOPE)
set(PAPI_FOUND ${PAPI_FOUND}
    PARENT_SCOPE)
set(PAPI_INCLUDE_DIRS ${PAPI_INCLUDE_DIRS}
    PARENT_SCOPE)
set(PAPI_LIBRARIES ${PAPI_LIBRARIES}
    PARENT_SCOPE)
set(ENABLE_SHARED_WINDOWS ${ENABLE_SHARED_WINDOWS}
    PARENT_SCOPE)

set (ADDITIONAL_COMPILE_FLAGS "-DDASH")
set (ADDITIONAL_INCLUDES      "")
set (ADDITIONAL_LIBRARIES     "")

if (ENABLE_UNIFIED_MEMORY_MODEL)
  set (ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_UNIFIED_MEMORY_MODEL")
endif()
if (ENABLE_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_LOGGING")
endif()
if (ENABLE_TRACE_LOGGING)
  set (ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_TRACE_LOGGING")
endif()
if (PAPI_FOUND)
  set (ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_PAPI")
  set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
       ${PAPI_INCLUDE_DIRS})
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       ${PAPI_LIBRARIES})
else()
  set (ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
       rt)
endif()

if (DART_IMPLEMENTATION_MPI_ENABLED)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/MPI.cmake)
endif()

foreach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
  set(DART_LIBRARY "dart-${dart_variant}")
  set(DASH_LIBRARY "dash-${dart_variant}")
  set(DART_TASK_LIBRARY "dart-tasking")
  set(VARIANT_ADDITIONAL_COMPILE_FLAGS
      "${ADDITIONAL_COMPILE_FLAGS} -DDART_IMPL_${dart_variant_caps}")
  set(VARIANT_ADDITIONAL_COMPILE_FLAGS
      "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -Wno-sign-compare")
  # Some benchmarks use MPI directly
  if (${dart_variant} STREQUAL "mpi")
    if (NOT ENABLE_SHARED_WINDOWS)
      set (VARIANT_ADDITIONAL_COMPILE_FLAGS
          "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -DDART_MPI_DISABLE_SHARED_WINDOWS")
    endif()
    set (VARIANT_ADDITIONAL_COMPILE_FLAGS
         "${VARIANT_ADDITIONAL_COMPILE_FLAGS} -DMPI_IMPL_ID=\"${MPI_IMPL_ID}\"")
    set (VARIANT_ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
         ${MPI_INCLUDE_PATH})
    set (VARIANT_ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES}
         ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  else()
    set (VARIANT_ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES})
    set (VARIANT_ADDITIONAL_LIBRARIES ${ADDITIONAL_LIBRARIES})
  endif()
  string(TOUPPER ${dart_variant} dart_variant_caps)
  message(INFO "Building DASH examples with backend " ${dart_variant_caps})
  SubDirList(
    DASH_EXAMPLES_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR})
  foreach(examplename ${DASH_EXAMPLES_DIRECTORIES})
    set(exampletarget ${examplename}.${dart_variant})
    if (EXISTS
       ${CMAKE_CURRENT_SOURCE_DIR}/${examplename}/main.cpp)

      # Build example for single DART implementation

      # Do not recurse into directories (GLOB_RECURSE) as these might contain
      # alternative implementations that have been added for documentation
      # purposes only:
      file(GLOB examplesources
           "${examplename}/*.c" "${examplename}/*.h"
           "${examplename}/*.cc" "${examplename}/*.cpp")
      add_executable(
        ${exampletarget}
        ${examplesources}
      )
      include_directories(
        ${CMAKE_SOURCE_DIR}/dash/include
        ${VARIANT_ADDITIONAL_INCLUDES}
      )
      target_link_libraries(
        ${exampletarget}
        ${DASH_LIBRARY}
        ${DART_TASK_LIBRARY}
        ${DART_LIBRARY}
        ${VARIANT_ADDITIONAL_LIBRARIES}
      )
      set_target_properties(
        ${exampletarget} PROPERTIES
        COMPILE_FLAGS ${VARIANT_ADDITIONAL_COMPILE_FLAGS}
      )
      set_target_properties(
        ${exampletarget} PROPERTIES
        CXX_STANDARD 14
      )
      set_target_properties(
        ${exampletarget} PROPERTIES
        CXX_STANDARD_REQUIRED 14
      )
      # Installation
      DeployBinary(${exampletarget})
      install(
        TARGETS ${exampletarget}
        DESTINATION bin/dash/examples/${dart_variant})
    endif()
  endforeach(examplename ${DASH_EXAMPLES_DIRECTORIES})
endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})

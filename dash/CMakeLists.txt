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
set(ENABLE_LOGGING ${ENABLE_LOGGING}
    PARENT_SCOPE)
set(ENABLE_TRACE_LOGGING ${ENABLE_TRACE_LOGGING}
    PARENT_SCOPE)
set(PAPI_FOUND ${PAPI_FOUND}
    PARENT_SCOPE)
set(PAPI_INCLUDE_DIRS ${PAPI_INCLUDE_DIRS}
    PARENT_SCOPE)
set(PAPI_LIBRARIES ${PAPI_LIBRARIES}
    PARENT_SCOPE)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_LIBRARY_SOURCES
     "src/*.c" "src/*.h" "src/*.cc" "src/*.cpp" )
file(GLOB_RECURSE DASH_LIBRARY_HEADERS 
     "include/*.h")
file(GLOB_RECURSE DASH_TEST_SOURCES
     "test/*.c" "test/*.h" "test/*.cc" "src/*.cpp")

# Directories containing the implementation of the library (-I):
set(DASH_LIBRARY_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
include_directories(
  ${DASH_LIBRARY_INCLUDE_DIRS}
  ${DASH_DART_IF_INCLUDE_DIR}
)

set (ADDITIONAL_COMPILE_FLAGS "-DDASH")
set (ADDITIONAL_INCLUDES      "")
set (ADDITIONAL_LIBRARIES     "")

if (MEMORY_MODEL_UNIFIED)
  set (ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDASH_UNIFIED_MEMORY_MODEL")
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

## Build targets

# DASH library for every enabled DART variant:
#
foreach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
  set(DART_LIBRARY "dart-${dart_variant}")
  set(DASH_LIBRARY "dash-${dart_variant}")
  string(TOUPPER ${dart_variant} dart_variant_caps)
  message(INFO "Building DASH library " ${DASH_LIBRARY})

  set (VARIANT_ADDITIONAL_COMPILE_FLAGS
       "${ADDITIONAL_COMPILE_FLAGS} -DDART_IMPL_${dart_variant_caps}")

  # Library compilation sources
  add_library(
    ${DASH_LIBRARY}         # library name
    ${DASH_LIBRARY_SOURCES} # sources
    ${DASH_LIBRARY_HEADERS} # headers
  )
  target_link_libraries(
    ${DASH_LIBRARY}           # library name
    ${DASH_LINK_DART_LIBRARY} # dependency: DART lib
    ${ADDITIONAL_LIBRARIES}
  )
  set_target_properties(
    ${DASH_LIBRARY} PROPERTIES
    COMPILE_FLAGS ${VARIANT_ADDITIONAL_COMPILE_FLAGS}
  )
  set_target_properties(
    ${DASH_LIBRARY} PROPERTIES
    CXX_STANDARD 11
  )
  set_target_properties(
    ${DASH_LIBRARY} PROPERTIES
    CXX_STANDARD_REQUIRED 11
  )
  include_directories(
    ${ADDITIONAL_INCLUDES}
  )
  if (${dart_variant} STREQUAL "mpi")
    include_directories(
      ${MPI_INCLUDE_PATH})
    target_link_libraries(
      ${DASH_LIBRARY}
      ${MPI_C_LIBRARIES})
  endif()

  DeployLibrary(${DASH_LIBRARY})

  # Install library
  install(TARGETS ${DASH_LIBRARY} DESTINATION lib)

endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})

## Tests
#
if (BUILD_TESTS)
  add_subdirectory(test)
endif()

## Examples
#
if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

## Installation

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/libdash.h
        DESTINATION include)

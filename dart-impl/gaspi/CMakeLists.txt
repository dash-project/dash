project(project_dash_dart_impl_gaspi)

# Library name
set(DASH_DART_IMPL_GASPI_LIBRARY dart-gaspi)
set(DASH_DART_BASE_LIBRARY dart-base)

include(${CMAKE_SOURCE_DIR}/CMakeExt/GASPI.cmake)

# Source- and header files to be compiled (OBJ):
file(GLOB_RECURSE DASH_DART_IMPL_GASPI_SOURCES "src/*.c" "src/*.h" "src/*.cc")
file(GLOB_RECURSE DASH_DART_IMPL_GASPI_HEADERS "include/*.h")

set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR}
    PARENT_SCOPE)

message("Use compiler: " "${CC}")
message("-- GASPI include path: " "${GASPI_INCLUDE_PATH}")

## Build targets

# Directories containing the implementation of the library (-I):
set(DASH_DART_IMPL_GASPI_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include_directories(
	${DASH_DART_IMPL_GASPI_INCLUDE_DIRS}
	${GASPI_INCLUDE_PATH}
	${DASH_DART_IF_INCLUDE_DIR}
	${DASH_DART_BASE_INCLUDE_DIR}
	${ADDITIONAL_INCLUDES}
)
# Library compilation sources
add_library(
	${DASH_DART_IMPL_GASPI_LIBRARY} # library name
	${DASH_DART_IMPL_GASPI_SOURCES} # sources
	${DASH_DART_IMPL_GASPI_HEADERS} # headers  
)

# Link dependencies
target_link_libraries(
  ${DASH_DART_IMPL_GASPI_LIBRARY}
  ${PRELOADED_LIBRARIES}
  ${GASPI_C_LIBRARIES}
  ${DASH_DART_BASE_LIBRARY}
  ${ADDITIONAL_LIBRARIES}
)

if(GASPI_COMPILE_FLAGS)
  set_target_properties(
    ${DASH_DART_IMPL_GASPI_LIBRARY} PROPERTIES
    COMPILE_FLAGS "${GASPI_COMPILE_FLAGS}")
endif()

if(GASPI_LINK_FLAGS)
  set_target_properties(
    ${DASH_DART_IMPL_GASPI_LIBRARY} PROPERTIES
    LINK_FLAGS "${GASPI_LINK_FLAGS}")
endif()

if(BUILD_DART_GASPI_TESTS)
    add_subdirectory(test)
endif()

## Installation

DeployLibrary(${DASH_DART_IMPL_GASPI_LIBRARY})

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
        DESTINATION include FILES_MATCHING PATTERN "*.h")
# Generated headers, if any
# install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/dash
#         DESTINATION include FILES_MATCHING PATTERN "*.h")
# Library
install(TARGETS ${DASH_DART_IMPL_GASPI_LIBRARY} DESTINATION lib)


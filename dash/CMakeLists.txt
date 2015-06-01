project(project_dash_libdash)

# Load global build settings
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} 
    PARENT_SCOPE)
set(DART_IMPLEMENTATIONS_LIST ${DART_IMPLEMENTATIONS_LIST} 
    PARENT_SCOPE)
set(BUILD_EXAMPLES ${BUILD_EXAMPLES}
    PARENT_SCOPE)
set(BUILD_TESTS ${BUILD_TESTS}
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

## Build targets

# DASH library for every enabled DART variant:
#
foreach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
  set(DART_LIBRARY "dart-${dart_variant}")
  set(DASH_LIBRARY "dash-${dart_variant}")
  message("-- Building DASH library " ${DASH_LIBRARY})

  if (${dart_variant} STREQUAL "mpi")
    set(ADD_INCLUDES ${MPI_INCLUDE_PATH})
    set(ADD_LIBS ${MPI_CXX_LIBRARIES})
  else()
    set(ADD_INCLUDES "")
    set(ADD_LIBS "")
  endif()

  include_directories(${ADD_INCLUDES})

  # Library compilation sources
  add_library(
    ${DASH_LIBRARY}         # library name
    ${DASH_LIBRARY_SOURCES} # sources
    ${DASH_LIBRARY_HEADERS} # headers
  )
  target_link_libraries(
    ${DASH_LIBRARY}           # library name
    ${DASH_LINK_DART_LIBRARY} # dep
    ${ADD_LIBS}               # dep
  )
  DeployLibrary(${DASH_LIBRARY})

  # Install library
  install(TARGETS ${DASH_LIBRARY} DESTINATION lib)

  # Tests
  # Examples
  if (BUILD_EXAMPLES)
    SubDirList(
      DASH_TEST_DIRECTORIES
      ${CMAKE_CURRENT_SOURCE_DIR}/examples)
    foreach(examplename ${DASH_TEST_DIRECTORIES})
      set(exampletarget ${examplename}.${dart_variant})
      if(IS_DIRECTORY
         ${CMAKE_CURRENT_SOURCE_DIR}/examples/${examplename})
        add_executable(
          ${exampletarget}
          examples/${examplename}/main.cpp
        )
        include_directories(
          ${CMAKE_SOURCE_DIR}/dash/include
        )
        target_link_libraries(
          ${exampletarget}
          ${DASH_LIBRARY}
          ${DART_LIBRARY})
        DeployBinary(${exampletarget})
        install(
          TARGETS ${exampletarget}
          DESTINATION bin/dash/examples/${dart_variant})
      endif()
    endforeach(examplename ${DASH_TEST_DIRECTORIES})
  endif()
endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})

## Tests
#
if (BUILD_TESTS)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/GoogleTest.cmake)
  foreach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
    set(DASH_LIBRARY "dash-${dart_variant}")
    set(DART_LIBRARY "dart-${dart_variant}")
    set(DASH_TEST "dash-test-${dart_variant}")
    include_directories(
      ${GTEST_INCLUDES}
      ${CMAKE_SOURCE_DIR}/dash/include
    )
    add_executable(
      ${DASH_TEST}
      ${DASH_TEST_SOURCES}
    )
    target_link_libraries(
      ${DASH_TEST}
      GTest
      ${DASH_LIBRARY}
      ${DART_LIBRARY}
    )
    DeployBinary(${DASH_TEST})
    install(
      TARGETS ${DASH_TEST}
      DESTINATION bin/dash/test/${dart_variant})
  endforeach(dart_variant ${DART_IMPLEMENTATIONS_LIST})
endif()

## Installation

# Headers
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/dash
          DESTINATION include FILES_MATCHING PATTERN "*.h")


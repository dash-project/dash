## Compiler flags

# GCC debug flags:
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table

set (DASH_DEVELOPER_COMPILE_FLAGS
     "-Weffc++ -Wcast-align -Wcast-qual -Wno-ctor-dtor-privacy "
     "-Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-declarations "
     "-Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual "
     "-Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo "
     "-Wstrict-overflow=5 -Wswitch -Wundef -Wno-unused -Wnon-virtual-dtor "
     "-Wreorder -Wdeprecated -Wno-float-equal")
 

set (CXX_WARN_FLAG "")
set (CC_WARN_FLAG  "")
if (ENABLE_COMPILER_WARNINGS)
  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
    # Flags for C and C++:
    set (CXX_WARN_FLAG "-Wall -Wextra -pedantic")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-function")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-missing-braces")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-format")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-parameter")
    set (CC_WARN_FLAG  "${CXX_WARN_FLAG}")
    # C++ specific flags:
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-invalid-offsetof")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-local-typedefs")
  endif()
endif()

# Set C++ compiler flags:
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang")
  # using Clang
  set (CXX_STD_FLAG "--std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_OMP_FLAG "-fopenmp")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CXX_STD_FLAG "--std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_GDB_FLAG "-ggdb3 -rdynamic"
       CACHE STRING "C++ compiler GDB debug symbols flag")
  set (CXX_OMP_FLAG "-fopenmp")
  if(ENABLE_LT_OPTIMIZATION)
    set (CXX_LTO_FLAG "-flto -fwhole-program -fno-use-linker-plugin")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CXX_STD_FLAG "-std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_OMP_FLAG "-qopenmp")
  if(ENABLE_LT_OPTIMIZATION)
    set (CXX_LTO_FLAG "-ipo")
  endif()
  if(ENABLE_CC_REPORTS)
    set (CC_REPORT_FLAG "-qopt-report=4 -qopt-report-phase ipo")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
  # Cray compiler not supported for C++
  message(FATAL_ERROR,
          "Cray compiler does not support C++11 features and is only "
          "eligible for building DART.")
endif()

# Set C compiler flags:
if ("${CMAKE_C_COMPILER_ID}" MATCHES ".*Clang")
  # using Clang
  set (CC_STD_FLAG "--std=c99"
       CACHE STRING "C compiler std flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CC_STD_FLAG "--std=c99"
       CACHE STRING "C compiler std flag")
  set (CC_GDB_FLAG "-ggdb3"
       CACHE STRING "C compiler GDB debug symbols flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CC_STD_FLAG "-std=c99"
       CACHE STRING "C compiler std flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Cray")
  # using Cray
  set (CC_STD_FLAG "-h c99"
       CACHE STRING "C compiler std flag")
endif()

set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_ENV_SETUP_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_ENV_SETUP_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} ${CXX_OMP_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -O0 -DDASH_DEBUG ${CC_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} ${CXX_OMP_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -O0 -DDASH_DEBUG ${CXX_GDB_FLAG}")

set(CMAKE_C_FLAGS_RELEASE
  "${CMAKE_C_FLAGS_RELEASE} ${CC_STD_FLAG} ${CXX_OMP_FLAG} ${CXX_LTO_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_STD_FLAG} ${CXX_OMP_FLAG} ${CXX_LTO_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")

if (BUILD_COVERAGE_TESTS)
  # Profiling is only supported for Debug builds:
  set(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
endif()

if (ENABLE_ASSERTIONS)
  set(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS -DDART_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS -DDART_ENABLE_ASSERTIONS")

  set(CMAKE_C_FLAGS_RELEASE
      "${CMAKE_C_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS -DDART_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS -DDART_ENABLE_ASSERTIONS")
endif()

message(STATUS "CC  flags (Debug):   ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CXX flags (Debug):   ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CC  flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CXX flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")


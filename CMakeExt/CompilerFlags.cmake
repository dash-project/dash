## Compiler flags

# GCC debug flags:
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table


find_package(OpenMP)


# The following warning options are intentionally not enabled:
#
#  ,--------------------------.-------------------------------------------.
#  | Flag                     | Reason                                    |
#  :--------------------------+-------------------------------------------:
#  | -Wmissing-declarations   | Arguably only relevant for code style     |
#  | -Wshadow                 | Very unlikely to cause unintended effects |
#  | -Weffc++                 | Spurious false positives                  |
#  '--------------------------'-------------------------------------------'

if (ENABLE_DEV_COMPILER_WARNINGS 
  OR ENABLE_EXT_COMPILER_WARNINGS 
  AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")

  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align")

  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set (DASH_DEVELOPER_CCXX_FLAGS
         "${DASH_DEVELOPER_CCXX_FLAGS} -Wopenmp-simd")
  endif()

  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wopenmp-simd")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-qual")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wdisabled-optimization -Wformat")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Winit-self")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wmissing-include-dirs -Wenum-compare")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wswitch")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wunused -Wtrigraphs")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wdeprecated -Wno-float-equal")

  if (OPENMP_FOUND)
    set (DASH_DEVELOPER_CCXX_FLAGS
         "${DASH_DEVELOPER_CCXX_FLAGS} -Wopenmp-simd")
  endif()

  # C++-only warning flags

  set (DASH_DEVELOPER_CXX_FLAGS "${DASH_DEVELOPER_CCXX_FLAGS}")

  set (DASH_DEVELOPER_CXX_FLAGS
         "${DASH_DEVELOPER_CXX_FLAGS} -Wno-ctor-dtor-privacy")

  if (ENABLE_EXT_COMPILER_WARNINGS)
    # this flag causes warnings on DASH_ASSERT_RETURNS
    set (DASH_DEVELOPER_CXX_FLAGS
         "${DASH_DEVELOPER_CXX_FLAGS} -Wsign-promo")
    # this flag might help spot overflows in index computation but is too
    # verbose in general
    set (DASH_DEVELOPER_CXX_FLAGS
         "${DASH_DEVELOPER_CXX_FLAGS} -Wstrict-overflow=2")
    # some good hints, but too style-related to be used in general
    set (DASH_DEVELOPER_CXX_FLAGS
         "${DASH_DEVELOPER_CXX_FLAGS} -Weffc++")
  endif()

  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Wreorder -Wnon-virtual-dtor")
  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Woverloaded-virtual")
  
  # C-only warning flags

  set (DASH_DEVELOPER_CC_FLAGS "${DASH_DEVELOPER_CCXX_FLAGS}")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wbad-function-cast")
  set (DASH_DEVELOPER_CC_FLAGS 
       "${DASH_DEVELOPER_CC_FLAGS}  -Wnested-externs")

  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set (DASH_DEVELOPER_CC_FLAGS
         "${DASH_DEVELOPER_CC_FLAGS}  -Wc99-c11-compat")
    set (DASH_DEVELOPER_CC_FLAGS
         "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-parameter-type")
  endif()

  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wpointer-sign")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-declarations")

  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
    set (DASH_DEVELOPER_CC_FLAGS
         "${DASH_DEVELOPER_CC_FLAGS} -diag-disable=10006")
    set (DASH_DEVELOPER_CXX_FLAGS
         "${DASH_DEVELOPER_CXX_FLAGS} -diag-disable=10006")
  endif()

endif()

# disable warnings on unknown warning flags 

set (CC_WARN_FLAG  "${CC_WARN_FLAG}  -Wall -Wextra -Wpedantic")
set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wall -Wextra -Wpedantic")

set (CC_WARN_FLAG  "${DASH_DEVELOPER_CC_FLAGS}")
set (CXX_WARN_FLAG "${DASH_DEVELOPER_CXX_FLAGS}")

if (ENABLE_DEV_COMPILER_WARNINGS)
  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
    # Flags for C and C++:
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-function")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-missing-braces")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-format")
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-parameter")
    set (CC_WARN_FLAG  "${CC_WARN_FLAG}")
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
  set (CXX_GDB_FLAG "-g"
       CACHE STRING "C++ compiler (clang++) debug symbols flag")
  set (CXX_OMP_FLAG ${OpenMP_CXX_FLAGS})
  set (CC_OMP_FLAG  ${OpenMP_CC_FLAGS})
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CXX_STD_FLAG "--std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_GDB_FLAG "-ggdb3 -rdynamic"
       CACHE STRING "C++ compiler GDB debug symbols flag")
  set (CXX_OMP_FLAG ${OpenMP_CXX_FLAGS})
  set (CC_OMP_FLAG  ${OpenMP_CC_FLAGS})
  if(ENABLE_LT_OPTIMIZATION)
    set (CXX_LTO_FLAG "-flto -fwhole-program -fno-use-linker-plugin")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CXX_STD_FLAG "-std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_OMP_FLAG ${OpenMP_CXX_FLAGS})
  set (CC_OMP_FLAG  ${OpenMP_CC_FLAGS})
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
  set (CC_GDB_FLAG "-g"
       CACHE STRING "C compiler (clang) debug symbols flag")
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
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} ${CC_OMP_FLAG}")
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG}")
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -Ofast -DDASH_DEBUG ${CC_GDB_FLAG}")

set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} ${CXX_OMP_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CC_REPORT_FLAG} ${CXX_WARN_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Ofast -DDASH_DEBUG ${CXX_GDB_FLAG}")


set(CMAKE_C_FLAGS_RELEASE
  "${CMAKE_C_FLAGS_RELEASE} ${CC_STD_FLAG} ${CC_OMP_FLAG}")
set(CMAKE_C_FLAGS_RELEASE
  "${CMAKE_C_FLAGS_RELEASE} ${CXX_LTO_FLAG} ${CC_REPORT_FLAG}")
set(CMAKE_C_FLAGS_RELEASE
  "${CMAKE_C_FLAGS_RELEASE} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")

set(CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_STD_FLAG} ${CXX_OMP_FLAG}")
set(CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_LTO_FLAG} ${CC_REPORT_FLAG}")
set(CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_WARN_FLAG} -Ofast -DDASH_RELEASE")

if (BUILD_COVERAGE_TESTS)
  # Profiling is only supported for Debug builds:
  set(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} --coverage -fprofile-arcs -ftest-coverage")
endif()

if (ENABLE_ASSERTIONS)
  set(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} -DDART_ENABLE_ASSERTIONS")

  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DDART_ENABLE_ASSERTIONS")

  set(CMAKE_C_FLAGS_RELEASE
      "${CMAKE_C_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_C_FLAGS_RELEASE
      "${CMAKE_C_FLAGS_RELEASE} -DDART_ENABLE_ASSERTIONS")

  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -DDART_ENABLE_ASSERTIONS")
endif()

message(STATUS "CC  flags (Debug):   ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CXX flags (Debug):   ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CC  flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CXX flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")


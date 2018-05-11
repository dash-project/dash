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

# set minimum requirements here
set (DART_C_STD_PREFERED "99")
set (DASH_CXX_STD_PREFERED "11")

# Used in CI Scripts to force a particular CXX version
if("$ENV{DART_FORCE_C_STD}")
  message(INFO "Force C STD $ENV{DART_FORCE_C_STD}")
  set(DART_C_STD_PREFERED "$ENV{DART_FORCE_C_STD}")

# Check if compiler provides c11
elseif(${CMAKE_VERSION} VERSION_GREATER 3.0.0)
  include(CheckCCompilerFlag)
  CHECK_C_COMPILER_FLAG("-std=c11" COMPILER_SUPPORTS_C11)
  if(COMPILER_SUPPORTS_C11)
    set (DART_C_STD_PREFERED "11")
    message(STATUS "Compile with C 11")
  endif()
endif()

# Same for C++
if("$ENV{DASH_FORCE_CXX_STD}")
  message(INFO "Force C++ STD $ENV{DASH_FORCE_CXX_STD}")
  set(DASH_CXX_STD_PREFERED "$ENV{DASH_FORCE_CXX_STD}")

# Check if compiler provides c++14
elseif(${CMAKE_VERSION} VERSION_GREATER 3.0.0)
  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
  if(COMPILER_SUPPORTS_CXX14)
    set (DASH_CXX_STD_PREFERED "14")
    message(STATUS "Compile with CXX 14")
  endif()
endif()

# Configure Compiler Warnings
if (ENABLE_DEV_COMPILER_WARNINGS
  OR ENABLE_EXT_COMPILER_WARNINGS
  AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")

  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align")

   if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND OPENMP_FOUND)
    set (DASH_DEVELOPER_CCXX_FLAGS
         "${DASH_DEVELOPER_CCXX_FLAGS} -Wopenmp-simd")
  endif()

  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align")
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
      "${DASH_DEVELOPER_CXX_FLAGS} -Weffc++ -Wno-error=effc++")
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


set (CXX_GDB_FLAG "-g"
     CACHE STRING "C++ compiler (clang++) debug symbols flag")
if(OPENMP_FOUND)
  set (CXX_OMP_FLAG ${OpenMP_CXX_FLAGS})
endif()

# Set C++ compiler flags:
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang")
  # using Clang
  set (CXX_STD_FLAG "--std=c++${DASH_CXX_STD_PREFERED}"
       CACHE STRING "C++ compiler std flag")

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.8.0")
    message(FATAL_ERROR "Insufficient Clang version (< 3.8.0)")
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CXX_STD_FLAG "--std=c++${DASH_CXX_STD_PREFERED}"
       CACHE STRING "C++ compiler std flag")
  set (CXX_GDB_FLAG "-ggdb3 -rdynamic")
  if(ENABLE_LT_OPTIMIZATION)
    set (CXX_LTO_FLAG "-flto -fwhole-program -fno-use-linker-plugin")
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.1")
    message(FATAL_ERROR "Insufficient GCC version (< 4.8.1)")
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CXX_STD_FLAG "-std=c++${DASH_CXX_STD_PREFERED}"
       CACHE STRING "C++ compiler std flag")
  if(ENABLE_LT_OPTIMIZATION)
    set (CXX_LTO_FLAG "-ipo")
  endif()
  if(ENABLE_CC_REPORTS)
    set (CC_REPORT_FLAG "-qopt-report=4 -qopt-report-phase ipo")
  endif()


  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "15.0.0")
    message(FATAL_ERROR "Insufficient Intel compiler version (< 15.0.0)")
  endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
  # Cray compiler not supported for C++
  message(FATAL_ERROR,
          "Cray compiler does not support C++11 features and is only "
          "eligible for building DART.")
endif()

set (CC_GDB_FLAG "-g"
     CACHE STRING "C compiler (clang) debug symbols flag")
if(OPENMP_FOUND)
  set (CC_OMP_FLAG  ${OpenMP_C_FLAGS})
endif()

# Set C compiler flags:
if ("${CMAKE_C_COMPILER_ID}" MATCHES ".*Clang")
  # using Clang
  set (CC_STD_FLAG "--std=c${DART_C_STD_PREFERED}"
       CACHE STRING "C compiler std flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CC_STD_FLAG "--std=c${DART_C_STD_PREFERED}"
       CACHE STRING "C compiler std flag")
  set (CC_GDB_FLAG "-ggdb3")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CC_STD_FLAG "-std=c${DART_C_STD_PREFERED}"
       CACHE STRING "C compiler std flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Cray")
  # using Cray
  set (CC_STD_FLAG "-h c${DART_C_STD_PREFERED}"
       CACHE STRING "C compiler std flag")
endif()

#if(${CMAKE_VERSION} VERSION_LESS 3.0.0 )
# CMake does not add compiler flags correctly, so use this workaround
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} ${CXX_STD_FLAG}")
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} ${CC_STD_FLAG}")
#endif()

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CXX_ENV_SETUP_FLAGS}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_OMP_FLAG}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_REPORT_FLAG} ${CC_WARN_FLAG}")
set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -O0 -DDASH_DEBUG ${CC_GDB_FLAG}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO
    "${CMAKE_C_FLAGS_RELWITHDEBINFO} ${CC_GDB_FLAG}")
set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CXX_LTO_FLAG}")
set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} -Ofast -DDASH_RELEASE")

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CXX_OMP_FLAG}")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CC_REPORT_FLAG} ${CXX_WARN_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDASH_DEBUG ${CXX_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CXX_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_LTO_FLAG}")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -DDASH_RELEASE")

if (BUILD_COVERAGE_TESTS)
  # Profiling is only supported for Debug builds:
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
endif()

if (ENABLE_ASSERTIONS)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -DDART_ENABLE_ASSERTIONS")

  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -DDART_ENABLE_ASSERTIONS")
endif()

message(STATUS "CC  flags          : ${CMAKE_C_FLAGS}")
message(STATUS "CXX flags          : ${CMAKE_CXX_FLAGS}")
message(STATUS "CC  flags (Debug)  : ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CXX flags (Debug)  : ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CC  flags (RelWDeb): ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
message(STATUS "CXX flags (RelWDeb): ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CC  flags (Release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CXX flags (Release): ${CMAKE_CXX_FLAGS_RELEASE}")


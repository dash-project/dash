## Compiler flags

# GCC debug flags:
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table



# The following warning options are intentionally not enabled:
#
#  ,--------------------------.-------------------------------------------.
#  | Flag                     | Reason                                    |
#  :--------------------------+-------------------------------------------:
#  | -Wmissing-declarations   | Arguably only relevant for code style     |
#  | -Wshadow                 | Very unlikely to cause unintended effects |
#  | -Weffc++                 | Spurious false positives                  |
#  '--------------------------'-------------------------------------------'

set(ENABLE_DEVELOPER_COMPILER_WARNINGS ${ENABLE_DEVELOPER_COMPILER_WARNINGS}
    PARENT_SCOPE)

if (ENABLE_DEVELOPER_COMPILER_WARNINGS)

  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align -Wcast-qual")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wdisabled-optimization -Wformat")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Winit-self -Wopenmp-simd")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wmissing-include-dirs -Wenum-compare")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wredundant-decls -Woverlength-strings")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wstrict-overflow=5 -Wswitch")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wno-unused -Wtrigraphs")
  set (DASH_DEVELOPER_CCXX_FLAGS
       "${DASH_DEVELOPER_CCXX_FLAGS} -Wdeprecated -Wno-float-equal")

  # C++-only warning flags

  set (DASH_DEVELOPER_CXX_FLAGS "${DASH_DEVELOPER_CCXX_FLAGS}")

  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Wno-ctor-dtor-privacy -Weffc++")
  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Wreorder -Wnon-virtual-dtor")
  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Wold-style-cast -Woverloaded-virtual")
  set (DASH_DEVELOPER_CXX_FLAGS
       "${DASH_DEVELOPER_CXX_FLAGS} -Wsign-promo")

  # C-only warning flags

  set (DASH_DEVELOPER_CC_FLAGS "${DASH_DEVELOPER_CCXX_FLAGS}")

  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wbad-function-cast -Wc99-c11-compat")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-prototypes -Wnested-externs")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wold-style-definition")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-parameter-type -Wpointer-sign")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-declarations")
  set (DASH_DEVELOPER_CC_FLAGS
       "${DASH_DEVELOPER_CC_FLAGS}  -Wstrict-prototypes")

endif()


set (CC_WARN_FLAG  "${DASH_DEVELOPER_CC_FLAGS}")
set (CXX_WARN_FLAG "${DASH_DEVELOPER_CXX_FLAGS}")

if (ENABLE_COMPILER_WARNINGS)
  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
    # Flags for C and C++:
    set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wall -Wextra -Wpedantic")
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


set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wa,-adhln=test-O3.s -g -fverbose-asm -masm=intel")

set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_DEBUG} -Wa,-adhln=test-O3.s -g -fverbose-asm -masm=intel")

set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_ENV_SETUP_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_ENV_SETUP_FLAGS}")

set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} ${CXX_OMP_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_DEBUG ${CC_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} ${CXX_OMP_FLAG} ${CC_REPORT_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_DEBUG ${CXX_GDB_FLAG}")

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


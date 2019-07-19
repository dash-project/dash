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
set (DASH_CXX_STD_PREFERED "14")

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
endif()

# Configure Compiler Warnings
if (ENABLE_DEV_COMPILER_WARNINGS
        OR ENABLE_EXT_COMPILER_WARNINGS
        AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")

    set (DASH_DEVELOPER_CXX_FLAGS
        "${DASH_DEVELOPER_CXX_FLAGS} -Wall -pedantic")
    set (DASH_DEVELOPER_CC_FLAGS
        "${DASH_DEVELOPER_CC_FLAGS} -Wall -pedantic")

    if (NOT ENABLE_EXT_COMPILER_WARNINGS)
        set (DASH_DEVELOPER_CXX_FLAGS
            "${DASH_DEVELOPER_CXX_FLAGS} -Werror")
        set (DASH_DEVELOPER_CC_FLAGS
            "${DASH_DEVELOPER_CC_FLAGS} -Werror")

        # Remove pedantic from Werror to allow unnamed union
        set (DASH_DEVELOPER_CC_FLAGS
            "${DASH_DEVELOPER_CC_FLAGS} -Wno-error=pedantic")

        # We ignore unused functions
        set (DASH_DEVELOPER_CXX_FLAGS
            "${DASH_DEVELOPER_CXX_FLAGS} -Wno-unused-function")
        # We further ignore format-errors due to logging
        set (DASH_DEVELOPER_CC_FLAGS
            "${DASH_DEVELOPER_CC_FLAGS} -Wno-format")

        if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
            set (DASH_DEVELOPER_CC_FLAGS
                "${DASH_DEVELOPER_CC_FLAGS} -Wno-format-pedantic")
        endif()

        # Prevent Clang from suggesting braces
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
            set (DASH_DEVELOPER_CXX_FLAGS
                "${DASH_DEVELOPER_CXX_FLAGS} -Wno-missing-braces")
        endif()

        # Also ignore signed/unsigned comparison
        set (DASH_DEVELOPER_CXX_FLAGS
            "${DASH_DEVELOPER_CXX_FLAGS} -Wno-sign-compare")
    endif()


    # For the extended warnings we disable -Werror and add additional compiler warnings

    if (ENABLE_EXT_COMPILER_WARNINGS)
        if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND OPENMP_FOUND)
            set (DASH_DEVELOPER_CCXX_FLAGS
                "${DASH_DEVELOPER_CCXX_FLAGS} -Wopenmp-simd")
        endif()

        # common warnings for both C and C++
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-align")
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wcast-qual")
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wdisabled-optimization")
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wmissing-include-dirs -Wenum-compare")
        # Enable all unused warnings
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wunused")
        set (DASH_DEVELOPER_CCXX_FLAGS
            "${DASH_DEVELOPER_CCXX_FLAGS} -Wdeprecated")

        # C-only warning flags

        set (DASH_DEVELOPER_CC_FLAGS "${DASH_DEVELOPER_CC_FLAGS} ${DASH_DEVELOPER_CCXX_FLAGS}")
        set (DASH_DEVELOPER_CC_FLAGS
            "${DASH_DEVELOPER_CC_FLAGS}  -Wbad-function-cast")
        set (DASH_DEVELOPER_CC_FLAGS
            "${DASH_DEVELOPER_CC_FLAGS}  -Wnested-externs")

        if (NOT "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
            set (DASH_DEVELOPER_CC_FLAGS
                "${DASH_DEVELOPER_CC_FLAGS}  -Wmissing-parameter-type")
        endif()

        # C++-only warning flags

        set (DASH_DEVELOPER_CXX_FLAGS "${DASH_DEVELOPER_CXX_FLAGS} ${DASH_DEVELOPER_CCXX_FLAGS}")

        if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
            # This disables unknown compiler options for Intel
            # including a lot of warnings from above
            set (DASH_DEVELOPER_CXX_FLAGS
                "${DASH_DEVELOPER_CXX_FLAGS} -diag-disable=10006")
        endif()
    endif()
endif()

if (NOT (ENABLE_DEV_COMPILER_WARNINGS
       OR ENABLE_EXT_COMPILER_WARNINGS)
    AND NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Cray")
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -Wno-format")
endif()

set (CXX_GDB_FLAG "-g"
    CACHE STRING "C++ compiler (clang++) debug symbols flag")
if(OPENMP_FOUND)
        #set (CXX_OMP_FLAG ${OpenMP_CXX_FLAGS})
endif()


# See https://en.cppreference.com/w/cpp/compiler_support for
# minimum versions
# Set C++ compiler flags:

# CMAKE prior to 3.10 detects IBM's XL as Clang.
# Check for the __ibmxl__ macro instead
CHECK_SYMBOL_EXISTS("__ibmxl__" "" HAVE_XLC_COMPILER)
if(HAVE_XLC_COMPILER)
    set (CXX_STD_FLAG "-std=c++${DASH_CXX_STD_PREFERED}"
        CACHE STRING "C++ compiler std flag")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang")
    # using Clang
    set (CXX_STD_FLAG "--std=c++${DASH_CXX_STD_PREFERED}"
        CACHE STRING "C++ compiler std flag")

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.4.0")
        message(FATAL_ERROR "Insufficient Clang version (< 3.4.0)")
    endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    # using GCC
    set (CXX_STD_FLAG "--std=c++${DASH_CXX_STD_PREFERED}"
        CACHE STRING "C++ compiler std flag")
    set (CXX_GDB_FLAG "-ggdb3 -rdynamic")
    if(ENABLE_LT_OPTIMIZATION)
        set (CXX_LTO_FLAG "-flto -fwhole-program -fno-use-linker-plugin")
    endif()

    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0.0")
        message(FATAL_ERROR "Insufficient GCC version (< 5.0.0)")
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


    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "17.0.0")
        message(FATAL_ERROR "Insufficient Intel compiler version (< 17.0.0)")
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
if(HAVE_XLC_COMPILER)
    set (CC_STD_FLAG "-std=c${DART_C_STD_PREFERED}"
        CACHE STRING "C compiler std flag")
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES ".*Clang")
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

if(${CMAKE_VERSION} VERSION_LESS 3.0.0 )
    # CMake does not add compiler flags correctly, so use this workaround
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} ${CXX_STD_FLAG}")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} ${CC_STD_FLAG}")
endif()

set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_ENV_SETUP_FLAGS}")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CXX_ENV_SETUP_FLAGS}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_OMP_FLAG}")
set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} ${CC_REPORT_FLAG} ${DASH_DEVELOPER_CC_FLAGS}")
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
    "${CMAKE_CXX_FLAGS} ${CC_REPORT_FLAG} ${DASH_DEVELOPER_CXX_FLAGS}")
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


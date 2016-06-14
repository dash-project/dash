## Compiler flags

# GCC debug flags:
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table

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
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CXX_STD_FLAG "-std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_OMP_FLAG "-qopenmp")
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
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} ${CC_WARN_FLAG} -Og -DDASH_DEBUG ${CC_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} ${CXX_OMP_FLAG} ${CC_WARN_FLAG} -Og -DDASH_DEBUG ${CXX_GDB_FLAG}")

set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CC_STD_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_STD_FLAG} ${CXX_OMP_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")

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

message(STATUS "CC  flags (debug):   ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CXX flags (debug):   ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CC  flags (release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CXX flags (release): ${CMAKE_CXX_FLAGS_RELEASE}")


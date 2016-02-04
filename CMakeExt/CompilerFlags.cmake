## Compiler flags

# GCC debug flags:
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table

message(INFO "C   compiler id:          ${CMAKE_C_COMPILER_ID}")
message(INFO "C++ compiler id:          ${CMAKE_CXX_COMPILER_ID}")

set (CXX_WARN_FLAG "")
set (CC_WARN_FLAG  "")
if (ENABLE_COMPILER_WARNINGS)
  set (CXX_WARN_FLAG "-Wall -pedantic")
  set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-unused-function")
  set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-missing-braces")
  set (CXX_WARN_FLAG "${CXX_WARN_FLAG} -Wno-format")
  set (CC_WARN_FLAG  "${CXX_WARN_FLAG}")
endif()

# Set C++ compiler flags:
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang")
  # using Clang
  set (CXX_STD_FLAG "--std=c++11"
       CACHE STRING "C++ compiler std flag")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  # using GCC
  set (CXX_STD_FLAG "--std=c++11"
       CACHE STRING "C++ compiler std flag")
  set (CXX_GDB_FLAG "-ggdb3 -rdynamic"
       CACHE STRING "C++ compiler GDB debug symbols flag")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  # using Intel C++
  set (CXX_STD_FLAG "-std=c++11"
       CACHE STRING "C++ compiler std flag")
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
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_DEBUG ${CC_GDB_FLAG}")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_DEBUG ${CXX_GDB_FLAG}")

set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CC_STD_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_STD_FLAG} ${CC_WARN_FLAG} -Ofast -DDASH_RELEASE")

if (ENABLE_ASSERTIONS)
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS")
endif()

message(STATUS "CC  flags (debug):   ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CXX flags (debug):   ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CC  flags (release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "CXX flags (release): ${CMAKE_CXX_FLAGS_RELEASE}")


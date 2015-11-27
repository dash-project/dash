
set (CXX_STD_FLAG "--std=c++11" CACHE STRING "C++ compiler std flag")

message(STATUS "Testing CMAKE_C_COMPILER (${CMAKE_C_COMPILER})")

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
  set (CC_STD_FLAG "--std=c11" CACHE STRING "C compiler std flag")
else()
# if ("${CMAKE_C_COMPILER}" MATCHES "cray" )
  set (CC_STD_FLAG "-h c99" CACHE STRING "C compiler std flag")
endif()

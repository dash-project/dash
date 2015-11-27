## Compiler flags

# GCC debug flags: 
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table


set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} ${CC_STD_FLAG} -Ofast -DDASH_DEBUG -ggdb3")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} ${CXX_STD_FLAG} -Ofast -DDASH_DEBUG -ggdb3 -rdynamic")

set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} ${CC_STD_FLAG} -Ofast -DDASH_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} ${CXX_STD_FLAG} -Ofast -DDASH_RELEASE")

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

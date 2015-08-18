## Compiler flags


set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Ofast -std=c99")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Ofast -std=gnu++11")

# GCC debug flags: 
# -ggdb       Debug info for GDB
# -rdynamic   Instructs the linker to add all symbols, not only used ones,
#             to the dynamic symbol table

set(CMAKE_C_FLAGS_DEBUG
    "${CMAKE_C_FLAGS_DEBUG} -DDASH_DEBUG -ggdb3")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -DDASH_DEBUG -ggdb3 -rdynamic")

set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} -DDASH_RELEASE")
set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -DDASH_RELEASE")

if (ENABLE_ASSERTIONS)
  set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -DDASH_ENABLE_ASSERTIONS")
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -DDASH_ENABLE_ASSERTIONS")
endif()

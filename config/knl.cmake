set(DASH_ENV_HOST_SYSTEM_ID "default" CACHE STRING
    "Host system type identifier")

if (NOT BUILD_GENERIC)
    if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU"
            OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    #set specific flags for clang or gcc to use avx-512
    endif()

    if ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel")
        set(CC_ENV_SETUP_FLAGS "${CC_ENV_SETUP_FLAGS} -xhost")
        set(CXX_ENV_SETUP_FLAGS "${CXX_ENV_SETUP_FLAGS} -xhost")
    endif()
endif()

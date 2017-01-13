## Flags to enabel support for multi-threading 

##
# At the moment, DART_ENABLE_THREADING also enables DART_THREADING_PTHREADS since
# Pthreads are the only threading implementation currently supported. 
##

set(ENABLE_THREADING ${ENABLE_THREADING}
    PARENT_SCOPE)

if (ENABLE_THREADING)

    # Find support for pthreads
    find_package(Threads REQUIRED)
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -pthread -DDART_ENABLE_THREADING -DDART_THREADING_PTHREADS")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -pthread -DDART_ENABLE_THREADING -DDART_THREADING_PTHREADS")
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()
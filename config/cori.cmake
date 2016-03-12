
set(DASH_ENV_HOST_SYSTEM_ID "supermuc" CACHE STRING
    "Host system type identifier")

if ("${CMAKE_CXX_COMPILER}" MATCHES "icc")
  message(NOTE "hwloc support is disabled for Intel programming environment on Cori")
  set (ENABLE_HWLOC OFF CACHE BOOL
       "MPI shared windows are disabled for IBM MPI on SuperMUC" FORCE)
  option(ENABLE_HWLOC OFF)
endif()

if (HWLOC_FOUND AND ENABLE_HWLOC)
  # Fixes order of linker flags which otherwise causes
  # linke error with Cray compiler:
  set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
       -lxml2 -lpciaccess)
endif()

if (ENABLE_MKL)
# link_directories(${MKL_LIBRARY_DIRS})
# set (ADDITIONAL_COMPILE_FLAGS
#      "${ADDITIONAL_COMPILE_FLAGS} -DDASH_ENABLE_MKL")
# set (ADDITIONAL_COMPILE_FLAGS
#      "${ADDITIONAL_COMPILE_FLAGS} -DMKL_LP64")
# set (ADDITIONAL_COMPILE_FLAGS
#      "${ADDITIONAL_COMPILE_FLAGS} -qopenmp")
# set (ADDITIONAL_INCLUDES ${ADDITIONAL_INCLUDES}
#      ${MKL_INCLUDE_DIRS})
# set (MKLROOT $ENV{MKLROOT})
# set (ADDITIONAL_LINK_FLAGS ${ADDITIONAL_LINK_FLAGS}
#      ${MKLROOT}/lib/intel64/libmkl_scalapack_lp64.a -Wl,--start-group ${MKLROOT}/lib/intel64/libmkl_intel_lp64.a ${MKLROOT}/lib/intel64/libmkl_core.a ${MKLROOT}/lib/intel64/libmkl_intel_thread.a ${MKLROOT}/lib/intel64/libmkl_blacs_intelmpi_lp64.a -Wl,--end-group -lpthread -lm -ldl -openmp)
endif()

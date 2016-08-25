# Prepare variables for the generation of StaticConfig.h

# all dash modules
set(DASH_MODULES PAPI HWLOC LIKWID NUMA PLASMA HDF5 MKL BLAS LAPACK SCALAPACK)

# set all variables to false as default
foreach(DASH_MODULE ${DASH_MODULES})
  set(CONF_AVAIL_${DASH_MODULE} "false")
endforeach()

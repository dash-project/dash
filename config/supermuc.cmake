# Configuration and CMake modules for SuperMUC

# Set compiler to Intel Compiler
#
# Or:
#   export CC=`which icc`
#   export CXX=`which icc`
#
set(ENV{CXX} icc)
set(ENV{CC} icc)

set(DASH_ENV_HOST_SYSTEM_ID "supermuc" CACHE STRING
    "Host system type identifier")


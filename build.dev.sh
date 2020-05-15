#!/bin/sh

BUILD_DIR=./build.dev

FORCE_BUILD=false
if [ "$1" = "-f" ]; then
  FORCE_BUILD=true
  shift
fi

await_confirm() {
  if ! $FORCE_BUILD; then
    echo ""
    echo "   To build using these settings, hit ENTER"
    read confirm
  fi
}

exit_message() {
  echo "--------------------------------------------------------"
  echo "Done. To install DASH, run:   make -C $BUILD_DIR install"
}

if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

# To specify a build configuration for a specific system, use:
#
#                    -DENVIRONMENT_TYPE=<type> \
#
# For available types, see the files in folder ./config.
# To specify a custom build configuration, use:
#
#                    -DENVIRONMENT_CONFIG_PATH=<path to cmake file> \
#

# To use an existing installation of gtest instead of downloading the sources
# from the google test subversion repository, use:
#
#                    -DGTEST_LIBRARY_PATH=${HOME}/gtest \
#                    -DGTEST_INCLUDE_PATH=${HOME}/gtest/include \
#

# To build with MKL support, set environment variables MKLROOT and INTELROOT.
#

# To enable IPM runtime support, use:
#
#                    -DIPM_PREFIX=<IPM install path> \

# To build with MKL support, set environment variables MKLROOT and INTELROOT.

# For likwid support, ensure that the likwid development headers are
# installed.

# Configure with default developer build settings:
mkdir -p $BUILD_DIR
rm -Rf $BUILD_DIR/*
(cd $BUILD_DIR && cmake -DCMAKE_BUILD_TYPE=Debug \
                        -DENVIRONMENT_TYPE=default \
                        -DINSTALL_PREFIX=$HOME/opt/dash-0.4.0-dev/ \
                        -DDART_IMPLEMENTATIONS=mpi \
                        -DENABLE_THREADSUPPORT=ON \
                        -DENABLE_DEV_COMPILER_WARNINGS=ON \
                        -DENABLE_EXT_COMPILER_WARNINGS=ON \
                        -DENABLE_LT_OPTIMIZATION=OFF \
                        -DENABLE_ASSERTIONS=ON \
                        \
                        -DENABLE_SHARED_WINDOWS=ON \
                        -DENABLE_DYNAMIC_WINDOWS=ON \
                        -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
                        \
                        -DENABLE_LOGGING=ON \
                        -DENABLE_TRACE_LOGGING=ON \
                        -DENABLE_DART_LOGGING=ON \
                        \
                        -DENABLE_LIBNUMA=ON \
                        -DENABLE_LIKWID=OFF \
                        -DENABLE_HWLOC=ON \
                        -DENABLE_PAPI=ON \
                        -DENABLE_MKL=ON \
                        -DENABLE_BLAS=ON \
                        -DENABLE_LAPACK=ON \
                        -DENABLE_SCALAPACK=ON \
                        -DENABLE_PLASMA=ON \
                        -DENABLE_HDF5=ON \
                        -DENABLE_MEMKIND=ON \
                        \
                        -DBUILD_EXAMPLES=ON \
                        -DBUILD_TESTS=ON \
                        -DBUILD_DOCS=OFF \
                        \
                        -DIPM_PREFIX=${IPM_HOME} \
                        -DPAPI_PREFIX=${PAPI_HOME} \
                        \
                        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                        "$@" \
                        ../ &&
 await_confirm &&
 make -j 4) && (cp $BUILD_DIR/compile_commands.json .) &&
exit_message


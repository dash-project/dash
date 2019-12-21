#!/bin/sh

BUILD_DIR=./build

FORCE_BUILD=false
if [ "$1" = "-f" ]; then
  FORCE_BUILD=true
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
  echo "Done. To install DASH, run    make install    in $BUILD_DIR"
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

# Configure with default release build settings:
mkdir -p $BUILD_DIR
rm -Rf $BUILD_DIR/*
(cd $BUILD_DIR && cmake -DCMAKE_BUILD_TYPE=Release \
                        -DENVIRONMENT_TYPE=default \
                        -DINSTALL_PREFIX=$HOME/opt/dash-0.4.0/ \
                        -DDART_IMPLEMENTATIONS=mpi \
                        -DENABLE_THREADSUPPORT=OFF \
                        -DENABLE_DEV_COMPILER_WARNINGS=OFF \
                        -DENABLE_EXT_COMPILER_WARNINGS=OFF \
                        -DENABLE_LT_OPTIMIZATION=OFF \
                        -DENABLE_ASSERTIONS=ON \
                        \
                        -DENABLE_SHARED_WINDOWS=OFF \
                        -DENABLE_DYNAMIC_WINDOWS=OFF \
                        -DENABLE_DEFAULT_INDEX_TYPE_LONG=OFF \
                        \
                        -DENABLE_LOGGING=OFF \
                        -DENABLE_TRACE_LOGGING=OFF \
                        -DENABLE_DART_LOGGING=OFF \
                        \
                        -DENABLE_LIBNUMA=OFF \
                        -DENABLE_LIKWID=OFF \
                        -DENABLE_HWLOC=OFF \
                        -DENABLE_PAPI=OFF \
                        -DENABLE_MKL=OFF \
                        -DENABLE_BLAS=OFF \
                        -DENABLE_LAPACK=OFF \
                        -DENABLE_SCALAPACK=OFF \
                        -DENABLE_PLASMA=OFF \
                        -DENABLE_HDF5=OFF \
                        -DENABLE_MEMKIND=OFF \
                        \
                        -DBUILD_EXAMPLES=ON \
                        -DBUILD_TESTS=OFF \
                        -DBUILD_DOCS=OFF \
                        \
                        -DIPM_PREFIX=${IPM_HOME} \
                        -DPAPI_PREFIX=${PAPI_HOME} \
                        \
                        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                        ../ && \
 await_confirm && \
 make -j 4) && (cp $BUILD_DIR/compile_commands.json .) && \
exit_message

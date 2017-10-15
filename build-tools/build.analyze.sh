#!/bin/sh

if ! [ -z ${SOURCING+x} ]; then

# This script runs scan-build to perform a static code analysis
# https://clang-analyzer.llvm.org/scan-build.html
#
# The actual compilers have to be set in the CCC prefix env. variables. e.g:
# export CCC_CC=clang-3.8
# export CCC_CXX=clang++3.8

## !! NOTE !!
#
#  See documentation of scan-build for details on recommended build
#  configuration:
#
#  https://clang-analyzer.llvm.org/scan-build.html#recommended_debug
#
##

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

# For likwid support, ensure that the likwid development headers are
# installed.


BUILD_DIR=build.analyze
REPORT_DIR=report            # relative to BUILD_DIR
BUILD_WRAPPER="${SCANBUILD_BIN}";

# try to find build wrapper
if [ "$BUILD_WRAPPER" = "" ]; then
  BUILD_WRAPPER="scan-build"
fi
if [ "$SCANBUILD_OPTS" = "" ]; then
  SCANBUILD_OPTS="-analyze-headers -plist-html"
fi
SCANBUILD_OPTS="-o $REPORT_DIR ${SCANBUILD_OPTS}"
SCANBUILD_OPTS="--force-analyze-debug-code -v ${SCANBUILD_OPTS}"

which $BUILD_WRAPPER ||
  (echo "This build requires $BUILD_WRAPPER. Set env. var SCANBUILD_BIN" \
   & exit 1);

# custom cmake command
CMAKE_COMMAND="$BUILD_WRAPPER $SCANBUILD_OPTS cmake3"

# default release build settings:
CMAKE_OPTIONS="         -DCMAKE_BUILD_TYPE=Debug \
                        -DENVIRONMENT_TYPE=default \
                        -DENABLE_COMPTIME_RED=OFF \
                        \
                        -DDART_IF_VERSION=3.2 \
                        -DINSTALL_PREFIX=$HOME/opt/dash-0.3.0/ \
                        -DDART_IMPLEMENTATIONS=mpi \
                        -DENABLE_THREADSUPPORT=ON \
                        -DENABLE_DEV_COMPILER_WARNINGS=OFF \
                        -DENABLE_EXT_COMPILER_WARNINGS=OFF \
                        -DENABLE_LT_OPTIMIZATION=OFF \
                        -DENABLE_ASSERTIONS=ON \
                        \
                        -DENABLE_SHARED_WINDOWS=ON \
                        -DENABLE_UNIFIED_MEMORY_MODEL=ON \
                        -DENABLE_DYNAMIC_WINDOWS=ON \
                        -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
                        \
                        -DENABLE_LOGGING=OFF \
                        -DENABLE_TRACE_LOGGING=OFF \
                        -DENABLE_DART_LOGGING=OFF \
                        \
                        -DENABLE_LIBNUMA=ON \
                        -DENABLE_LIKWID=OFF \
                        -DENABLE_HWLOC=ON \
                        -DENABLE_PAPI=ON \
                        -DENABLE_MKL=OFF \
                        -DENABLE_BLAS=ON \
                        -DENABLE_LAPACK=ON \
                        -DENABLE_SCALAPACK=ON \
                        -DENABLE_PLASMA=ON \
                        -DENABLE_HDF5=ON \
                        \
                        -DBUILD_EXAMPLES=OFF \
                        -DBUILD_TESTS=OFF \
                        -DBUILD_DOCS=OFF \
                        \
                        -DIPM_PREFIX=${IPM_HOME} \
                        -DPAPI_PREFIX=${PAPI_HOME} \
                        \
                        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

# the make command used
MAKE_COMMAND="$BUILD_WRAPPER $SCANBUILD_OPTS make"

# the install command used
# use a noop command if the built version is not useful to install
INSTALL_COMMAND=":"

else

  $(dirname $0)/build.sh $(echo $0 | sed "s/.*build.\(.*\).sh/\1/") $@

fi


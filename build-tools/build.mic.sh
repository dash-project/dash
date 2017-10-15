#!/bin/sh

if ! [ -z ${SOURCING+x} ]; then

# To use an existing installation of gtest instead of downloading the sources
# from the google test subversion repository, use:
#
#                    -DGTEST_LIBRARY_PATH=${HOME}/gtest \
#                    -DGTEST_INCLUDE_PATH=${HOME}/gtest/include \

# To specify a build configuration for a specific system, use:
#
#                    -DENVIRONMENT_TYPE=<type> \
#
# For available types, see the files in folder ./config.
# To specify a custom build configuration, use:
#
#                    -DENVIRONMENT_CONFIG_PATH=<path to cmake file> \

# To build with MKL support, set environment variables MKLROOT and INTELROOT.

# relative to $ROOTDIR of dash
BUILD_DIR=build.mic

# custom cmake command
CMAKE_COMMAND="cmake"

# default release build settings for supermic environment:
CMAKE_OPTIONS="         -DCMAKE_BUILD_TYPE=Release \
                        -DENVIRONMENT_TYPE=supermic \
                        -DCMAKE_C_COMPILER=mpiicc \
                        -DCMAKE_CXX_COMPILER=mpiicc \
                        -DINSTALL_PREFIX=$HOME/opt/dash-0.3.0-mic/ \
                        -DDART_IMPLEMENTATIONS=mpi \
                        -DENABLE_THREADSUPPORT=OFF \
                        -DENABLE_DEV_COMPILER_WARNINGS=OFF \
                        -DENABLE_EXT_COMPILER_WARNINGS=OFF \
                        -DENABLE_ASSERTIONS=OFF \
                        \
                        -DENABLE_SHARED_WINDOWS=ON \
                        -DENABLE_DYNAMIC_WINDOWS=ON \
                        -DENABLE_UNIFIED_MEMORY_MODEL=ON \
                        -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
                        \
                        -DENABLE_LOGGING=OFF \
                        -DENABLE_TRACE_LOGGING=OFF \
                        -DENABLE_DART_LOGGING=OFF \
                        \
                        -DENABLE_LIBNUMA=OFF \
                        -DENABLE_LIKWID=OFF \
                        -DENABLE_HWLOC=OFF \
                        -DENABLE_PAPI=OFF \
                        -DENABLE_MKL=ON \
                        -DENABLE_BLAS=OFF \
                        -DENABLE_LAPACK=OFF \
                        -DENABLE_SCALAPACK=OFF \
                        -DENABLE_PLASMA=OFF \
                        -DENABLE_HDF5=OFF \
                        \
                        -DBUILD_EXAMPLES=ON \
                        -DBUILD_TESTS=OFF \
                        -DBUILD_DOCS=OFF \
                        \
                        -DPAPI_PREFIX=${PAPI_HOME} \
                        -DIPM_PREFIX=${IPM_BASE}"

# the make command used
MAKE_COMMAND="make -j 5"

# the install command used
# use a noop command if the built version is not useful to install
INSTALL_COMMAND="make install"

else

  $(dirname $0)/build.sh mic $@

fi


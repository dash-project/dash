#!/bin/sh

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
  echo "Done. To install DASH, run    make install    in ./build"
}

if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

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

# Configure with default release build settings:
mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake -DCMAKE_BUILD_TYPE=Release \
                     -DENVIRONMENT_TYPE=default \
                     -DDART_IF_VERSION=3.2 \
                     -DINSTALL_PREFIX=$HOME/opt/ \
                     -DDART_IMPLEMENTATIONS=mpi,shmem \
                     -DENABLE_ASSERTIONS=ON \
                     -DENABLE_SHARED_WINDOWS=ON \
                     -DENABLE_UNIFIED_MEMORY_MODEL=ON \
                     -DENABLE_LOGGING=OFF \
                     -DENABLE_TRACE_LOGGING=OFF \
                     -DENABLE_DART_LOGGING=OFF \
                     -DBUILD_EXAMPLES=ON \
                     -DBUILD_TESTS=ON \
                     -DPAPI_PREFIX=${PAPI_HOME} \
                     ../ && \
 await_confirm && \
 make) && \
exit_message

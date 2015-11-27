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

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake -DCMAKE_BUILD_TYPE=Debug \
                     -DENABLE_ASSERTIONS=ON \
                     -DDART_IF_VERSION=3.2 \
                     -DINSTALL_PREFIX=$HOME/opt/ \
                     -DDART_IMPLEMENTATIONS=mpi,shmem \
                     -DBUILD_EXAMPLES=ON \
                     -DENABLE_LOGGING=ON \
                     -DENABLE_TRACE_LOGGING=ON \
                     -DENABLE_DART_LOGGING=ON \
                     -DENABLE_SHARED_MEMORY=ON \
                     -DMEMORY_MODEL_UNIFIED=ON \
                     -DBUILD_TESTS=ON \
                     -DPAPI_PREFIX=${PAPI_HOME} \
                     ../ && \
 await_confirm && \
 make) && \
exit_message

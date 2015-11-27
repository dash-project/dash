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

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake -DCMAKE_BUILD_TYPE=Release \
                     -DENABLE_ASSERTIONS=ON \
                     -DDART_IF_VERSION=3.2 \
                     -DINSTALL_PREFIX=$HOME/opt/ \
                     -DDART_IMPLEMENTATIONS=mpi,shmem \
                     -DBUILD_EXAMPLES=ON \
                     -DENABLE_LOGGING=OFF \
                     -DENABLE_TRACE_LOGGING=OFF \
                     -DENABLE_DART_LOGGING=OFF \
                     -DENABLE_SHARED_MEMORY=OFF \
                     -DMEMORY_MODEL_UNIFIED=ON \
                     -DBUILD_TESTS=ON \
                     -DPAPI_PREFIX=${PAPI_HOME} \
                     ../ && \
 await_confirm && \
 make) && \
exit_message

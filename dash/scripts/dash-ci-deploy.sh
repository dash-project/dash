#!/bin/sh

if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

# Parse args:
BASEPATH=`git rev-parse --show-toplevel`
FORCE_BUILD=false
DO_INSTALL=false
INSTALL_PATH=""
BUILD_TYPE="Release"
MAKE_TARGET=""
MAKE_PROCS=`cat /proc/cpuinfo | grep --count 'processor'`
while [ "$#" -gt 0 ]; do
  case "$1" in
    -f)    FORCE_BUILD=true;
           shift 1;;
    --b=*) BUILD_TYPE="${1#*=}";
           shift 1;;
    --j=*) MAKE_PROCS="${1#*=}";
           shift 1;;
    --i=*) MAKE_TARGET="install";
           INSTALL_PATH="${1#*=}";
           DO_INSTALL=true;
           shift 1;;
  esac
done

if [ "$INSTALL_PATH" = "" ]; then
  echo "Install path must be set with --i=/install/path"
  exit -1
fi

await_confirm() {
  if ! $FORCE_BUILD; then
    echo    "$BUILD_SETTINGS"
    echo    ""
    read -p "   To build using these settings, hit ENTER"
  fi
}

exit_message() {
  echo "----------------------------------------------------"
  if $DO_INSTALL; then
    echo "Done. DASH has been installed to $INSTALL_PATH"
  else
    echo "Done. To install DASH, run  make install  in ./build"
  fi
}

if [ "$BUILD_TYPE" = "Release" ]; then
  BUILD_SETTINGS="
  -DCMAKE_BUILD_TYPE=Release \
  -DENVIRONMENT_TYPE=default \
  -DDART_IF_VERSION=3.2 \
  -DINSTALL_PREFIX=$INSTALL_PATH \
  -DDART_IMPLEMENTATIONS=mpi \
  -DENABLE_ASSERTIONS=OFF \
  -DENABLE_SHARED_WINDOWS=ON \
  -DENABLE_UNIFIED_MEMORY_MODEL=ON \
  -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
  -DENABLE_LOGGING=OFF \
  -DENABLE_TRACE_LOGGING=OFF \
  -DENABLE_DART_LOGGING=OFF \
  -DENABLE_SCALAPACK=OFF \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON \
  -DPAPI_PREFIX=${PAPI_HOME}"
elif [ "$BUILD_TYPE" = "Debug" ]; then
  BUILD_SETTINGS="
  -DCMAKE_BUILD_TYPE=Debug \
  -DENVIRONMENT_TYPE=default \
  -DDART_IF_VERSION=3.2 \
  -DINSTALL_PREFIX=$INSTALL_PATH \
  -DDART_IMPLEMENTATIONS=mpi \
  -DENABLE_ASSERTIONS=ON\
  -DENABLE_SHARED_WINDOWS=ON \
  -DENABLE_UNIFIED_MEMORY_MODEL=ON \
  -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
  -DENABLE_LOGGING=OFF \
  -DENABLE_TRACE_LOGGING=OFF \
  -DENABLE_DART_LOGGING=OFF \
  -DENABLE_SCALAPACK=OFF \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON \
  -DPAPI_PREFIX=${PAPI_HOME}"
elif [ "$BUILD_TYPE" = "Development" ]; then
  BUILD_SETTINGS="
  -DCMAKE_BUILD_TYPE=Debug \
  -DENVIRONMENT_TYPE=default \
  -DDART_IF_VERSION=3.2 \
  -DINSTALL_PREFIX=$INSTALL_PATH \
  -DDART_IMPLEMENTATIONS=mpi \
  -DENABLE_ASSERTIONS=ON \
  -DENABLE_SHARED_WINDOWS=ON \
  -DENABLE_UNIFIED_MEMORY_MODEL=ON \
  -DENABLE_DEFAULT_INDEX_TYPE_LONG=ON \
  -DENABLE_LOGGING=ON \
  -DENABLE_TRACE_LOGGING=ON \
  -DENABLE_DART_LOGGING=ON \
  -DENABLE_SCALAPACK=OFF \
  -DBUILD_EXAMPLES=ON \
  -DBUILD_TESTS=ON \
  -DPAPI_PREFIX=${PAPI_HOME}"
fi

alias nocolor="sed 's/\x1b\[[0-9;]*m//g'"

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake $BASEPATH $BUILD_SETTINGS | nocolor && \
 await_confirm && \
 make -j $MAKE_PROCS VERBOSE=1 $MAKE_TARGET) && \
exit_message

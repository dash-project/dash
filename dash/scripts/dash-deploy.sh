#!/bin/sh

# Parse args:
BASEPATH=`git rev-parse --show-toplevel`
FORCE_BUILD=false
DO_INSTALL=false
INSTALL_PATH=""
BUILD_TYPE="Debug"
MAKE_TARGET=""
MAKE_PROCS=`cat /proc/cpuinfo | grep --count 'processor'`
while [ "$#" -gt 0 ]; do
  case "$1" in
    -f)    FORCE_BUILD=true;
           shift 1;;
    -r)    BUILD_TYPE="Release";
           shift 1;;
    -d)    BUILD_TYPE="Debug";
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

function await_confirm
{
  if ! $FORCE_BUILD; then
    echo    "$BUILD_SETTINGS"
    echo    ""
    read -p "   To build using these settings, hit ENTER"
  fi
}

function exit_message {
  echo "----------------------------------------------------"
  if $DO_INSTALL; then
    echo "Done. DASH has been installed to $INSTALL_PATH"
  else
    echo "Done. To install DASH, run  make install  in ./build"
  fi
}

if [ "$BUILD_TYPE" = "Release" ]; then
  BUILD_SETTINGS=" 
  -DCMAKE_BUILD_TYPE=Release
  -DENABLE_ASSERTIONS=OFF
  -DDART_IF_VERSION=3.2
  -DINSTALL_PREFIX=$INSTALL_PATH
  -DDART_IMPLEMENTATIONS=mpi,shmem
  -DBUILD_EXAMPLES=ON
  -DENABLE_LOGGING=OFF
  -DENABLE_TRACE_LOGGING=OFF
  -DBUILD_TESTS=ON"
elif [ "$BUILD_TYPE" = "Debug" ]; then
  BUILD_SETTINGS="
  -DCMAKE_BUILD_TYPE=Debug
  -DENABLE_ASSERTIONS=ON
  -DDART_IF_VERSION=3.2
  -DINSTALL_PREFIX=$INSTALL_PATH
  -DDART_IMPLEMENTATIONS=mpi,shmem
  -DBUILD_EXAMPLES=ON
  -DENABLE_LOGGING=ON
  -DENABLE_TRACE_LOGGING=ON
  -DBUILD_TESTS=ON"
fi

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake $BASEPATH $BUILD_SETTINGS && \
 await_confirm && \
 make -j $MAKE_PROCS $MAKE_TARGET) && \
exit_message

#!/usr/bin/env bash

if [ -z "$1" ]
  then
    echo "No argument supplied! Using default ($HOME)"    
    PARENT_DIR=$HOME
  else
    PARENT_DIR=$1
fi

export DASH_HOME=`pwd`
export DASH_INCLUDES=$PARENT_DIR/include
export DASH_LIBS=$PARENT_DIR/lib
export CPATH=$DASH_INCLUDES
export LD_LIBRARY_PATH=$DASH_LIBS


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
mkdir -p $DASH_INCLUDES
mkdir -p $DASH_LIBS
export CPATH=$DASH_INCLUDES
export LD_LIBRARY_PATH=$DASH_LIBS

if [ ! -d "$DASH_INCLUDES/gtest" ]; then
    echo ' '
    echo "---> unzipping google test lib into $PARENT_DIR"
    echo ' '
    unzip gtest-1.6.0.zip -d $PARENT_DIR
    echo ' '
    echo "---> configure"
    echo ' '
    cd $PARENT_DIR/gtest-1.6.0 && ./configure 
    echo ' '
    echo "---> calling make"
    echo ' '
    make 
    cd -
    echo ' '
    echo "---> copying generated files..."
    echo ' '
    cp -r $PARENT_DIR/gtest-1.6.0/include/gtest $DASH_INCLUDES
    cp $PARENT_DIR/gtest-1.6.0/lib/.libs/libgtest.a $DASH_LIBS
fi

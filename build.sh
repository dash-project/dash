#!/bin/sh

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake -DCMAKE_BUILD_TYPE=Release \
                     -DENABLE_ASSERTIONS=OFF \
                     -DDART_IF_VERSION=3.1 \
                     -DINSTALL_PREFIX=$HOME/opt/ \
                     -DDART_IMPLEMENTATIONS=mpi,shmem \
                     -DBUILD_EXAMPLES=ON \
                     -DENABLE_LOGGING=ON \
                     -DENABLE_TRACE_LOGGING=OFF \
                     -DBUILD_TESTS=ON ../ && \
make)

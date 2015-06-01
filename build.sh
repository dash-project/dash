#!/bin/sh

mkdir -p build
rm -Rf ./build/*
(cd ./build && cmake -DDART_IF_VERSION=3.1 \
                     -DINSTALL_PREFIX=$HOME/opt/ \
                     -DDART_IMPLEMENTATIONS=mpi,shmem \
                     -DBUILD_EXAMPLES=ON \
                     -DBUILD_TESTS=ON ../ && \
make)


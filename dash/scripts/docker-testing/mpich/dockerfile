# Dockerfile for the DASH project
FROM          ubuntu:latest

MAINTAINER    The DASH Team <team@dash-project.org>

RUN           apt-get update -y    \
           && apt-get install -y   \
                  git              \
                  build-essential  \
                  libz-dev         \
                  cmake            \
                  uuid-runtime     \
                  libhwloc-plugins \
                  libhwloc-dev     \
                  clang-5.0

WORKDIR       /tmp
ENV           CC=gcc
ENV           CXX=g++

ADD         http://icl.utk.edu/projects/papi/downloads/papi-5.6.0.tar.gz papi.tgz
RUN           tar -xf papi.tgz
RUN           cd papi*/src/                     \
              && ./configure --prefix=/opt/papi \
              && make                           \
              && make install
ENV           PAPI_BASE=/opt/papi

# MPICH 3.2
ADD           http://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1.tar.gz mpich.tgz
RUN           tar -xf mpich.tgz
RUN           cd mpich*                           \
           && ./configure --prefix=/opt/mpich     \
                          --enable-threads        \
                          --disable-fortran       \
           && make                                \
           && make install
ENV           PATH=${PATH}:/opt/mpich/bin

# PHDF5 1.10
ADD           https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.2/src/hdf5-1.10.2.tar.gz phdf5.tgz
RUN           tar -xf phdf5.tgz
RUN           cd hdf5*                        \
           && CC=/opt/mpich/bin/mpicc         \
              ./configure --enable-parallel   \
                          --prefix=/opt/phdf5 \
           && make                            \
           && make install
ENV           HDF5_BASE=/opt/phdf5

ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PAPI_BASE}/lib:/opt/mpich/lib:${HDF5_BASE}/lib
ENV           VERBOSE_CI='true'

# DEBUG: enable core dumps 
RUN           ulimit -c unlimited

# Set workdir to dash home
WORKDIR       /opt/dash

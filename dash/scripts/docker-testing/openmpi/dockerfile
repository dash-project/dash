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
                  clang-3.8

WORKDIR       /tmp
ENV           CC=gcc
ENV           CXX=g++

# Compiler error when compiling with -pedantic for papi >= 5.5
 ADD         http://icl.utk.edu/projects/papi/downloads/papi-5.5.1.tar.gz papi.tgz
#ADD           http://icl.cs.utk.edu/projects/papi/downloads/papi-5.4.3.tar.gz papi.tgz
RUN           tar -xf papi.tgz
RUN           cd papi*/src/                     \
              && ./configure --prefix=/opt/papi \
              && make                           \
              && make install
ENV           PAPI_BASE=/opt/papi

# Openmpi 1.10.5
ADD           https://www.open-mpi.org/software/ompi/v1.10/downloads/openmpi-1.10.7.tar.gz ompi.tgz
RUN           tar -xf ompi.tgz
RUN           cd openmpi*                         \
           && ./configure --prefix=/opt/openmpi  \
                           --enable-mpi-thread-multiple \
           && make                               \
           && make install
ENV           PATH=${PATH}:/opt/openmpi/bin
ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/openmpi/lib

# PHDF5 1.10
ADD           https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.1/src/hdf5-1.10.1.tar.gz phdf5.tgz
RUN           tar -xf phdf5.tgz
RUN           cd hdf5*                        \
           && CC=/opt/openmpi/bin/mpicc       \
              ./configure --enable-parallel   \
                          --prefix=/opt/phdf5 \
           && make                            \
           && make install
ENV           HDF5_BASE=/opt/phdf5

ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PAPI_BASE}/lib:${HDF5_BASE}/lib
ENV           MPI_EXEC_FLAGS='--allow-run-as-root'
ENV           VERBOSE_CI='true'

# Set workdir to dash home
WORKDIR       /opt/dash

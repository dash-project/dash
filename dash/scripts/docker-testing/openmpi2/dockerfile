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

# Install PAPI from source
WORKDIR       /tmp

ADD           http://icl.utk.edu/projects/papi/downloads/papi-5.6.0.tar.gz papi.tgz
RUN           tar -xf papi.tgz
RUN           cd papi*/src/                     \
              && ./configure --prefix=/opt/papi \
              && make                           \
              && make install

# Openmpi 2.1
ADD           https://www.open-mpi.org/software/ompi/v2.1/downloads/openmpi-2.1.3.tar.gz ompi.tgz
RUN           tar -xf ompi.tgz
RUN           cd openmpi*                         \
           && ./configure --prefix=/opt/openmpi   \
                          --enable-mpi-thread-multiple \
           && make                                \
           && make install
ENV           PATH=${PATH}:/opt/openmpi/bin
ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/openmpi/lib

# PHDF5 1.10
ADD           https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.2/src/hdf5-1.10.2.tar.gz phdf5.tgz
RUN           tar -xf phdf5.tgz
RUN           cd hdf5*                        \
           && CC=/opt/openmpi/bin/mpicc       \
              ./configure --enable-parallel   \
                          --prefix=/opt/phdf5 \
           && make                            \
           && make install
ENV           HDF5_BASE=/opt/phdf5

# Prepare env
ENV           PAPI_BASE=/opt/papi
ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PAPI_BASE}/lib:${HDF5_BASE}/lib
ENV           VERBOSE_CI='true'

# Set workdir to dash home
WORKDIR       /opt/dash

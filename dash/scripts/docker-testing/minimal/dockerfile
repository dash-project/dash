# Dockerfile for the DASH project
# This container provides an environment for our minimal system requirements
FROM          ubuntu:latest

MAINTAINER    The DASH Team <team@dash-project.org>

RUN           apt-get update -y    \
           && apt-get install -y   \
                  git              \
                  build-essential  \
                  libz-dev         \
                  cmake            \
                  gcc-5            \
                  uuid-runtime

# Install PAPI from source
WORKDIR       /tmp

ADD           http://icl.utk.edu/projects/papi/downloads/papi-5.5.1.tar.gz papi.tgz
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
           && make -j 4                               \
           && make install
ENV           PATH=${PATH}:/opt/openmpi/bin
ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/openmpi/lib

# Prepare env
ENV           PAPI_BASE=/opt/papi
ENV           LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PAPI_BASE}/lib:${HDF5_BASE}/lib
ENV           VERBOSE_CI='true'

# Set workdir to dash home
WORKDIR       /opt/dash

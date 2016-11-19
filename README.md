[![CI Status](https://circleci.com/gh/dash-project/dash.svg?style=shield&circle-token=cd221e93159f5c97477c9699f3b7adc54d344ae6)](https://circleci.com/dashboard)
[![Build Status](https://travis-ci.org/dash-project/dash.svg?branch=development)](https://travis-ci.org/dash-project/dash) [![Documentation Status](https://readthedocs.org/projects/dash/badge/?version=latest)](http://dash.readthedocs.io/en/latest/?badge=latest) [![Documentation](https://codedocs.xyz/dash-project/dash.svg)](https://codedocs.xyz/dash-project/dash/)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/491/badge)](https://bestpractices.coreinfrastructure.org/projects/491)


DASH
====

A C++ Template Library for Distributed Data Structures with Support
for Hierarchical Locality for HPC and Data-Driven Science.

Summary
-------

Exascale systems are scheduled to become available in 2018-2020 and will be
characterized by extreme scale and a multilevel hierarchical organization.

Efficient and productive programming of these systems will be a challenge,
especially in the context of data-intensive applications. Adopting the 
promising notion of Partitioned Global Address Space (PGAS) programming the
DASH project develops a data-structure oriented C++ template library that
provides hierarchical PGAS-like abstractions for important data containers
(multidimensional arrays, lists, hash tables, etc.) and allows a developer
to control (and explicitly take advantage of) the hierarchical data layout
of global data structures.

In contrast to other PGAS approaches such as UPC, DASH does not propose a
new language or require compiler support to realize global address space 
semantics. Instead, operator overloading and other advanced C++ features
are used to provide the semantics of data residing in a global and
hierarchically partitioned address space based on a runtime system with
one-sided messaging primitives provided by MPI or GASNet.

As such, DASH can co-exist with parallel programming models already in
widespread use (like MPI) and developers can take advantage of DASH by
incrementally replacing existing data structures with the implementation
provided by DASH. Efficient I/O directly to and from the hierarchical
structures and DASH-optimized algorithms such as map-reduce are also part
of the project. Two applications from molecular dynamics and geoscience
are driving the project and are adapted to use DASH in the course of the
project.


Funding
-------

DASH is funded by the German Research Foundation (DFG) under the priority
programme "Software for Exascale Computing - SPPEXA" (2013-2018).


Community
---------

**Project Website:**

http://www.dash-project.org

**GitHub:**

https://github.com/dash-project

**Repository:**

SSH: https://github.com/dash-project/dash.git
HTTPS: git@github.com:dash-project/dash.git

**Contact:**

team@dash-project.org



Installation
============


Docker Containers
-----------------

For pre-build Docker container images, see the
[DASH project on Docker Hub](https://hub.docker.com/r/dashproject)


Building from Source
--------------------

DASH is build using CMake.

Build scripts are provided for typical DASH configurations and can serve
as starting points for custom builds:


Script file name | Description 
---------------- | --------------------------------------
`build.sh`       | Standard release build
`build.dev.sh`   | Development / debug build
`build.mic.sh`   | Release build for Intel MIC (Xeon Phi)


### Prerequisites

- CMake version 2.8.5 or greater (3.0.0 or greater recommended)
- C compiler supporting the C99 standard
- C++ compiler supporting the C++11 standard

Optional third-party libraries directly supported by DASH:

- PAPI
- libnuma
- hwloc
- BLAS implementation like Intel MKL, ATLAS
- LIKWID
- HDF5


### Building DASH from Source

To build the DASH project using CMake with default build settings, run:

    (dash/)$ cmake --build .

Or, specify a new directory used for the build:

    (dash/)$ mkdir build && cd ./build

For a list of available CMake parameters:

    (build/)$ cmake .. -L

Parameters can be set using `-D` flags. As an example, these parameters
will configure the build process to use icc as C compiler instead of the
default compiler:

    (build/)$ cmake -DCMAKE_C_COMPILER=icc ..

To configure build parameters using ccmake:

    (build/)$ ccmake ..


### 1. Choosing a DASH runtime (DART)

DASH provides the following variants:

  - MPI: the Message Passing Interface
  - CUDA: nNvidia's Compute Unified Device Architecture (contributor
    distribution only)
  - SHMEM: Symmetric Hierarchical Memory access (contributor distribution
    only)

The build process creates the following libraries:

  - libdart-mpi
  - libdart-cuda
  - libdart-shmem

By default, DASH is configured to build all variants of the runtime.
You can specify which implementations of DART to build using the cmake
option

    (build/)$ cmake -DDART_IMPLEMENTATIONS=mpi,shmem ...

Programs using DASH select a runtime implementation by linking against the
respective library.


### 3. Examples and Unit Tests

Source code of usage examples of DASH are located in `dash/examples/`.
Examples each consist of a single executable and are built by default.
Binaries from examples and unit tests are deployed to the build direcory
but will not be installed.
To disable building of examples, specify the cmake option

    (build/)$ cmake -DBUILD_EXAMPLES=OFF ...

To disable building of unit tests, specify the cmake option

    (build/)$ cmake -DBUILD_TESTS=OFF ...

The example applications are located in the bin/ folder in the build
directory.


### 4. Installation

The default installation path is `$HOME/opt` as users on HPC systems
typically have install permissions in their home directory only.

To specify a different installation path, use

    (build/)$ cmake -DINSTALL_PREFIX=/your/install/path ../

The option `-DINSTALL_PREFIX=<DASH install path>` can also be given in Step 1.

The installation process copies the 'bin', 'lib', and 'include' directories
in the build directory to the specified installation path.

    (dash/)$ cmake --build . --target install

Or manually using make:

    (build/)$ cmake <build options> ../
    (build/)$ make
    (build/)$ make install


Running DASH Applications
-------------------------

With the MPI variant, applications are spawn by MPI:

    $ mpirun <MPI args> <app>-mpi

For CUDA and SHMEM, use

    $ dartrun-cuda <dartrun-args> <app>-cuda

and respectively

    $ dartrun-shmem <dartrun-args> <app>-shmem


Running Tests
-------------

Launch the DASH unit test suite using <code>dash-test-shmem</code> or
<code>dash-test-mpi</code>:

    (dash/shmem/bin/)$ dartrun-shmem <dartrun args> dash-test-shmem <gtest args>

or

    (dash/mpi/bin/)$ mpirun <MPI args> dash-test-mpi <gtest args>


For example, you would all unit tests of matrix data structures on 4 units
using the MPI runtime with:

    (dash/mpi/bin/)$ mpirun -n 4 dash-test-mpi --gtest_filter="MatrixTest*"

or all tests except for the Array test suite:

    (dash/mpi/bin/)$ mpirun -n 4 dash-test-mpi --gtest_filter="-ArrayTest*"



Profiling DASH Applications using IPM
-------------------------------------

Use `LD_PRELOAD` to run a DASH application built with the DART-MPI backend:

    $ LD_PRELOAD=/$IPM_HOME/lib/libipm.so mpirun -n <nproc> <DASH executable>

Available options for IPM are documented in the
[IPM user guide](http://ipm-hpc.org/docs/user.php).


Links
-----

The DASH project homepage: http://www.dash-project.org 

The Munich Network Management homepage: http://www.mnm-team.org


Developer Notes
===============

This section is only relevant to contributors in the DASH project.


Code Style
----------

We follow the
[Google C++ Style Guide](http://google.github.io/styleguide/cppguide.html)
which is widely accepted in established open source projects.

The
[standards defined by the LLVM team](http://llvm.org/docs/CodingStandards.html)
are worth mentioning, too.



Contributing Code
-----------------

The DASH software development process is kept as simple as possible,
balancing pragmatism and QA.

**1. Create a new, dedicated branch for any task.**

We follow the naming convention:

- `feat-<issue-id>-<shortname>` for implementing and testing features
- `bug-<issue-id>-<shortname>`  for bugfix branches
- `sup-<issue-id>-<shortname>`  for support tasks (e.g. build system,
  														  documentation)

For example, when fixing a bug in team allocation, you could name your branch
*bug-team-alloc*.

**2. Submit an issue on GitHub**

- State the name of your branch in the ticket description.
- Choose appropriate tags (support, bug, feature, ...).
- As a member in the DASH project, assign the ticket to the owner of the
  affected module.


**3. For features and bugfixes, implement unit tests**

The integration test script can be used to run tests in Debug and
Release builds with varying number of processes automatically:

    (dash-root)$ ./dash/scripts/dash-ci.sh

Check if CI succeeded on
[Travis](https://travis-ci.org/dash-project/dash)
and
[CircleCI](https://circleci.com/gh/dash-project/dash).


**4. Once you are finished with your implementation and unit tests:**

- Create a new pull request on GitHub.
- Assign the pull request to one or more members in the DASH team
  for review.


**5. Reviewing**

After you merged your changes to master, choose a reviewer:

- Edit the ticket in Redmine
- Set state to "Resolved"
- Set "Assigned to" to the team member that will review your code

For now, we chose to merge to development branch before reviewing so
everyone can contribute to development snapshots without depending on other
team members.

**6. Closing a ticket**

Tickets are closed by reviewers once the related pull request passed
review and has been merged.



**7. Merging to master**

The development branch of DASH is merged to master periodically once all
issues in the development branch associated with the next stable release
are closed.

Before merging:

- Update CHANGELOG in development branch.
- Announce the merge: send the updated changelog to the DASH mailing
  list and set a deadline for review, testing, and pending issue reports.

After merging:

- Tag the merge commit with the version number of the release, e.g.
  `dash-0.5.0`
- Increment the DASH version number in the development branch, e.g from
  0.5.0 to 0.6.0.
- Publish a new release: create a tarball distribution by running
 `release.pl` and add a link on the DASH project website.

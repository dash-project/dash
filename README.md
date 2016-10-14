
[![Build Status](https://travis-ci.org/dash-project/dash.svg?branch=development)](https://travis-ci.org/dash-project/dash) [![Documentation Status](https://readthedocs.org/projects/dash/badge/?version=latest)](http://dash.readthedocs.io/en/latest/?badge=latest) [![Documentation](https://codedocs.xyz/dash-project/dash.svg)](http://doc.dash-project.org/api/latest/html/)

DASH
====

A C++ Template Library for Distributed Data Structures with Support
for Hierarchical Locality for HPC and Data-Driven Science

Build and Installation
----------------------

To build the DASH project using CMake, run:

    (dash/)$ cmake --build .

Or, specify a new directory used for the build:

    (dash/)$ mkdir build && cd ./build

For a list of available CMake parameters:

    (build/)$ cmake . -L

Parameters can be set using `-D` flags. As an example, these parameters
will configure the build process to use icc as C compiler instead of the
default compiler:

    (build/)$ cmake -DCMAKE_C_COMPILER=icc

Optionally, configure build parameters using ccmake:

    (build/)$ ccmake .

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

### 2. Developer Settings

*Only in DASH contributor distribution*

To activate a previous version of the DART interface, specify the
cmake parameter

    (build/)$ cmake -DDART_INTERFACE_VERSION=x.y ...

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

The default installation path is `$HOME/opt` as users on HPC systems typically
have install permissions in their home directory only.

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

    $ mpirun <app>-mpi

For CUDA and SHMEM, use

    $ dartrun-cuda <app>-cuda

and respectively

    $ dartrun-shmem <app>-shmem


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

The DASH software development process is kept as simple as possible, balancing
pragmatism and QA.

**1. Create a new, dedicated branch for any task.**

We follow the naming convention:

- `feat-<shortname>` for implementing and testing features
- `bug-<shortname>` for bugfix branches
- `sup-<shortname>` for support tasks (e.g. build system, documentation)

For example, when fixing a bug in team allocation, you could name your branch
*bug-team-alloc*.

**2. Create a ticket in Redmine**

- State the name of your branch in the ticket description.
- Set the ticket type (Support, Bug, or Feature)
- Assign the ticket to yourself

There is no need for time tracking, we use Redmine to maintain an overview
of active branches and who is working on which component.

**3. For features and bugfixes, implement unit tests**

The integration test script can be used to run tests in Debug and
Release builds with varying number of processes automatically:

    (dash-root)$ ./dash/scripts/dash-ci.sh

**4. Once you are finished with your implementation and unit tests:**

- Clone branch master into a new working copy:

        $ git clone git@git.dash-project.org:dash -b master ./dash-master

- In the master working copy, pull from your branch:

        (dash-master)$ git pull origin feat-myfeature

- Run continuous integration suite in the updated master working copy:

        (dash-master)$ ./dash/scripts/dash-ci.sh

- If continuous integration passed, push to master:

        (dash-master)$ git push origin master

**5. Reviewing**

After you merged your changes to master, choose a reviewer:

- Edit the ticket in Redmine
- Set state to "Resolved"
- Set "Assigned to" to the team member that will review your code

For now, we chose to merge to development branch before reviewing so
everyone can contribute to development snapshots without depending on other
team members.

**6. Closing a ticket**

Tickets are closed by reviewers once the code changes in the ticket's
branch passed review.

Branches are only deleted when their ticket is closed.

**7. Merging to master**

The development branch of DASH is merged to master periodically once all
tickets in the development branch are closed.

Before merging:

- Update the changelog in development branch.
- Announce the merge: send the updated changelog to the DASH mailing
  list and set a deadline for issue reports.

After merging:

- Increment the DASH version number in the development
  branch.
- Publish a new release: create a tarball distribution by running
 `release.pl` and add a link on the DASH project website.


Running Tests
-------------

Launch the DASH unit test suite using <code>dash-test-shmem</code> or
<code>dash-test-mpi</code>:

    (bin/dash/test/shmem)$ dartrun-shmem <dartrun args> dash-test-shmem <gtest args>

or

    (bin/dash/test/mpi)$ mpirun <MPI args> dash-test-mpi <gtest args>

For example, you would all unit tests of matrix data structures on 4 units
using the MPI runtime with:

    (bin/dash/test/mpi)$ mpirun -n 4 dash-test-mpi --gtest_filter=Matrix*


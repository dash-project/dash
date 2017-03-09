# Continuous Integration
Taking care of the various differences in HPC software and architectures is not an easy task. For a seamless integration of both new features and bugfixes DASH provides support for continuous integration (CI).
If CI passes, there is a high change that your changes do not hurt other parts of the code. However always keep in mind that a CI is only able to falsify but not to verify code.

## Unit Tests
For unit testing DASH uses the widely used GoogleTest framework.
For each feature of DASH there should be a corresponding unit test `dash/test`. It is recommended that the developer of the feature does not write the tests himself.
Whether the tests should be build is set in the build script using `-DBUILD_TESTS=ON`. The testsuite can then be easily executed using `mpirun -n <procs> ./build/bin/dash-test-mpi`. The output should be self explaining.

### Code Coverage
Code coverage tests are useful to verify if the whole public API is covered by the unit tests.

#### Dependencies

-   gcc / g++
-   gcov
-   lcov

#### How to get the results?

``` {.bash}
./build.cov.sh

cd build.cov
# build tests
make
# execute coverage measurement and generate reports
make coverage

# open results in browser
firefox coverage/index.html
```

If some tests do not work with just one unit, they can be excluded using
the filter string. Multiple tests can be separated by “:”. The pattern
is as follows:

``` {.bash}
# export GTEST_FILTER="POSTIVE_PATTERNS[-NEGATIVE_PATTERNS]"

# Run all tests which start with S or L,
# but not LocalRangeTest.LargeArray and not SUMMA tests
export GTEST_FILTER="S*:L*:-LocalRangeTest.LargeArray:SUMMA*"

# more common example
export GTEST_FILTER="-LocalRangeTest.LargeArray"
```

#### CMake Bug

The best solution would be to use the coverage integration in cmake by
setting `CMAKE\_BUILD\_TYPE=Coverage`. This is not possible as the
coverage flags are inserted before the `-fopenmp` flag. In modern
versions of gcc this is not allowed and leads to an linker error.

Currently `CMAKE\_BUILD\_TYPE=Profile` is used.

## CI Scripts

The DASH CI scripts located in `dash/scripts` automate the process of building DASH in various configuration and executing the tests.
There are three main scripts:

- `dash-ci-deploy.sh`: deploys various DASH configurations.
- `dash-test.sh`: runs a given target with a varying number of nodes and parses the output.
- `dash-ci.sh`: calls `dash-ci-deploy.sh` and executes the tests using `dash-test.sh`.

### Deployment 

Each CI configuration is identified by `$BUILD_TYPE`. If you intend to add another config, just add another case in the if/else statement.
The `$BUILD_SETTINGS` holds the settings which are passed to cmake. When building, the build of each target is written to a folder named like the target.
For example, the `Minimal` target creates a folder `dash-ci/Minimal` where dash is build and installed.

There are some environment variables that are used to modify the build settings. This is especially useful to customize or speedup the deployment if using the online CI providers.
If you intend to add another environment variable always add a default value if it is not set.

- `$DASH_BUILDEX="ON"|"OFF"` specifies if the examples should be build in this deployment run.
- `$DASH_MAKE_PROCS` max number of parallelism in make. Limit this value to reduce memory consumption during compilation. If not set the number of available processors is used.

### Execute Tests
In `dash-test.sh` the environment is checked and for example OpenMPI specific settings like `--map-by-core` can be set. After that the tests are executed using a specified set of mpi processes per run. This is specified using `run_suite <nprocs>`.
There are never more processes spin up than the host provides CPU cores. Furthermore some characteristics can be specified using environment variables:

- `$DASH_MAX_UNITS` use at most this number of processes.
- `$MPI_EXEC_FLAGS` pass these flags to the mpirun command.

### Execute CI 

Call `dash-ci.sh <target> ..` to run the CI for a list of targets. The target configuration has to be set in the deployment script. If no targets are given, a list of default targets is build and executed.
If the logs of each target contain no errors, the CI return exit code 0, otherwise 1.

## CI Environments

To simulate different common environments DASH uses Docker containers. For further information on Docker see the [vendor documentation](https://docs.docker.com/). The containers are build using the dockerfiles located in `dash/scripts/docker-testing/<env>`. This can be done either locally or by using the pre build images located on [Dockerhub](https://hub.docker.com/u/dashproject/). The containers hosted on Dockerhub are automatically build from the development branch of the official DASH repository.

### Build from Dockerfile

A docker container image named `dashproject/ci-testing:<env>` can be build using the following command. `env` has to be substituted with the name of the environment. 

```
# build container
docker build --tag dashproject/ci-testing:<env> dash/scripts/docker-testing/<env>
# alternatively pull official image
docker pull dashproject/ci-testing:<env>
```

!!! Note

    These containers can also be used as a good starting point for developing DASH applications.

As the containers only provide an environment but no DASH installation, a DASH repository should be mounted as shared folder.
The following command starts an interactive container with DASH located in /opt/dash, assumed that the command is at the top level of the DASH repository.

```bash
# mount current folder to /opt/dash
docker run -it -v $(pwd):/opt/dash dashproject/ci-testing:<env>
```

## Online CI providers

Currently we use the two online CI providers [Travis](https://travis-ci.org/dash-project/) and [CircleCI](https://circleci.com/gh/dash-project). The main testing logic is equal for both providers and follows this scheme:

1. pull docker image for specified env
2. run tests inside docker
3. analyze logfiles for errors
4. copy artefacts / print output messages

!!! Note

    As the exit code of a Docker container is not reliable, the output is parsed for errors outside docker. This is done by the `run-docker.sh` script.

To limit the complexity in the yml files, each ci folder (`dash/scripts/<ci>`) contains shell scripts for building / pulling and starting the docker containers. The file names should be self-explaining.
In the `run-docker.sh` file are also the environment variables for the Docker container set. These are then accessed by the `dash-ci.sh` script which is executed inside the container.
If you intend to run only a subset of build targets in a specific environment, just skip the environment loop:

```bash
if [ "$MPIENV" == "openmpi2_vg" ] && [ "$BUILD_CONFIG" != "Debug" ]; then
  echo "Skipping target $BUILD_CONF in ENV $MPIENV"
  continue
fi
```

The following command is taken from `travisci/run-docker.sh` and annotated to explain the basic logic:
```bash
docker run -v $PWD:/opt/dash dashproject/ci:$MPIENV /bin/sh -c "export DASH_MAKE_PROCS='4'; <...> sh dash/scripts/dash-ci.sh $BUILD_CONFIG | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"
#           |                           |              |                      |                               |                                    |
#       mount DASH          use container named ...  execute sh as shell      |                       execute CI script                   Do not ouput Log lines
#                                                                        set env variables 
```

### Travis

This provider is used for compilation tests (including examples) and supports direct checking of pull requests. Also the openmpi environment is tested. The commands are specified in `.travis.yml`.

### CircleCI

The main advantage of CircleCI is that multiple (up to 4) environments can be tested in parallel which reduces the test time significantly.
Another advantage is that the xml output of googletest is parsed and the failed tests are presented nicely.
The basic logic is specified in `circle.yml`.

The `run-docker.sh` file is worth taking a closer look at. In `$MPIENVS` a list of environments is specified. These are then evenly split over the available CircleCI instances. If more environments are specified than instances available, the containers are executed sequentially. This affects only the time for the CI to complete, but has no effect on the test conditions and results.

After the completion of each Docker container, the test results are gathered and copied to the artifacts directory. The directory/file structure is as follows: `<env>/<build_type>/dash-test-<nprocs>.xml`. For further information on CircleCI artifacts see the [official documentation](https://circleci.com/docs/build-artifacts/).

#### Debugging
CircleCI supports direct debugging inside the CI container / VM. Therfor click on the `Debug via SSH` button on a running build and ssh into the desired container. As we use Docker inside CircleCI to run our tests, all pathes printed in the CI output refer to internal pathes inside the Docker container.

!!! Note

    Attaching to a running container is problematic, as the containers are not run in interactive mode. Hence your terminal might hang.

The best way to debug is to spin up a interactive container using the corresponding environment. For example, if a problem occured in env `openmpi2` use the following command to start the container. As the current working directory is mounted to `/opt/dash`, run the command inside the DASH repository folder.

```bash
docker run -it -v $( pwd ):/opt/dash dashproject/ci:openmpi2
```

Inside the container, `cd` to `/opt/dash` and execute `/bin/bash /opt/dash/dash/dash/scripts/dash-ci.sh` to run the CI. If you are only interessted in a single target, pass it to the CI as described above: `dash-ci.sh $TARGET`.

To leave the container again, just type `exit`.

!!! Note

    Your SSH access is automatically terminated after 30 minutes.

#!/bin/bash

MPIENVS=(mpich openmpi2)
BUILD_CONFIG=$1
COMPILER=$2
MAKE_PROCS=4

if [[ "$COMPILER" == "clang" ]]; then
  C_COMPILER="clang-3.8"
  CXX_COMPILER="clang++-3.8"
  MAKE_PROCS=2
else
  COMPILER="gnu"
  C_COMPILER="gcc"
  CXX_COMPILER="g++"
fi

DASH_ENV_EXPORTS="export DASH_MAKE_PROCS='${MAKE_PROCS}'; export DASH_MAX_UNITS='3'; export DASH_BUILDEX='OFF';"

DASH_ENV_EXPORTS="${DASH_ENV_EXPORTS} export CC='${C_COMPILER}'; export CXX='${CXX_COMPILER}';"

# run tests
i=0
for MPIENV in ${MPIENVS[@]}; do
  if [[ $(( $i % ${CIRCLE_NODE_TOTAL} )) -eq ${CIRCLE_NODE_INDEX} ]]; then

    echo "Starting docker container: $MPIENV"
    # prevent ci timeout by printing message every 1 minute
    while sleep 120; do echo "[ CI ] Timeout prevention $(date)"; done &

    docker run -v $PWD:/opt/dash dashproject/ci:$MPIENV \
                  /bin/sh -c "$DASH_ENV_EXPORTS sh dash/scripts/dash-ci.sh $BUILD_CONFIG | tee dash-ci.log 2> dash-ci.err;"

    # stop timeout prevention
    kill $!

    # upload xml test-results
    mkdir -p $CIRCLE_TEST_REPORTS/$MPIENV/$COMPILER/$BUILD_CONFIG
    cp ./build-ci/*/$COMPILER/$BUILD_CONFIG/dash-tests-*.xml $CIRCLE_TEST_REPORTS/$MPIENV/$COMPILER/$BUILD_CONFIG/
    # upload build and test logs 
    mkdir -p $CIRCLE_TEST_REPORTS/$MPIENV/$COMPILER/$BUILD_CONFIG/logs
    cp ./build-ci/*/$COMPILER/$BUILD_CONFIG/*.log $CIRCLE_TEST_REPORTS/$MPIENV/$COMPILER/$BUILD_CONFIG/logs

    echo "checking logs"

    cat $PWD/dash-ci.log | grep -i "FAILED\|ERROR"
    if [ "$?" -eq "0" ]; then
      echo "Log contains errors"
      exit 1
    fi
  fi
  i=$((i + 1))
done

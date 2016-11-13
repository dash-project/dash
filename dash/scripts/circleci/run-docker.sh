#!/bin/bash

MPIENVS=(mpich openmpi openmpi2)
BUILD_CONFIG=$1

# run tests
i=0
for MPIENV in ${MPIENVS[@]}; do
  if [[ $(( $i % ${CIRCLE_NODE_TOTAL} )) -eq ${CIRCLE_NODE_INDEX} ]]; then
#    # specify combinations which are not inteded to run here.
#
#    if [ "$MPIENV" == "openmpi2_vg" ]; then
#      # run valgrind container only if target is debug
#      if [ "$BUILD_CONFIG" != "Debug" ]; then
#        echo "Skipping target $BUILD_CONFIG in ENV $MPIENV"
#        i=$((i + 1))
#        continue
#      fi
#    else # all other containers
#      # skip debug target
#      if [ "$BUILD_CONFIG" == "Debug" ]; then
#        echo "Skipping target $BUILD_CONFIG in ENV $MPIENV"
#        i=$((i + 1))
#        continue
#      fi
#    fi

    echo "Starting docker container: $MPIENV"

    docker run -v $PWD:/opt/dash dashproject/ci:$MPIENV /bin/sh -c "export DASH_MAKE_PROCS='4'; export DASH_MAX_UNITS='3'; export DASH_BUILDEX='OFF'; sh dash/scripts/dash-ci.sh $BUILD_CONFIG | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"

    mkdir -p $CIRCLE_TEST_REPORTS/$MPIENV/$BUILD_CONFIG
    cp ./build-ci/*/$BUILD_CONFIG/dash-tests-*.xml $CIRCLE_TEST_REPORTS/$MPIENV/$BUILD_CONFIG/

    echo "checking logs"

    cat $PWD/dash-ci.log | grep "FAILED"
    if [ "$?" -eq "0" ]; then
      echo "Log contains errors"
      exit 1
    fi
  fi
  i=$((i + 1))
done

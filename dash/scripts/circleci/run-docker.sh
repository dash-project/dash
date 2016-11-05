#!/bin/bash

MPIENVS=(openmpi mpich)

# run tests 
i=0
for MPIENV in ${MPIENVS[@]}; do
  if [[ $(( $i % ${CIRCLE_NODE_TOTAL} )) -eq ${CIRCLE_NODE_INDEX} ]]; then
    
    echo "Starting docker container: $MPIENV"
    
    docker run -v $PWD:/opt/dash dashproject/ci-testing:$MPIENV /bin/sh -c "export DASH_MAKE_PROCS='4'; export DASH_MAX_UNITS='3'; export DASH_BUILDEX='OFF'; sh dash/scripts/dash-ci.sh | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"
    
    TARGETS=`ls -d ./build-ci/*/* | xargs -n1 basename`
    for TARGET in $TARGETS; do
      mkdir -p $CIRCLE_TEST_REPORTS/$MPIENV/$TARGET
      cp ./build-ci/*/$TARGET/dash-tests-*.xml $CIRCLE_TEST_REPORTS/$MPIENV/$TARGET/
    done
    
    echo "checking logs"
    
    cat $PWD/dash-ci.log | grep "FAILED"
    if [ "$?" -eq "0" ]; then
      echo "Log contains errors"
      exit 1
    fi
  fi
  
  i=$((i + 1))
done

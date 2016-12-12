#!/bin/sh

TIMESTAMP=`date +%Y%m%d-%H%M%S`
BASEPATH=`git rev-parse --show-toplevel`
CMD_DEPLOY=$BASEPATH/dash/scripts/dash-ci-deploy.sh
CMD_TEST=$BASEPATH/dash/scripts/dash-test.sh
FAILED=false

run_ci()
{
  BUILD_TYPE=${1}
  BUILD_UUID=`uuidgen | awk -F '-' '{print $1}'`

  DEPLOY_PATH=$BASEPATH/build-ci/${TIMESTAMP}--uuid-${BUILD_UUID}/${BUILD_TYPE}

  mkdir -p $DEPLOY_PATH && \
    cd $DEPLOY_PATH

  echo "[-> BUILD  ] Deploying $BUILD_TYPE build to $DEPLOY_PATH ..."

  if [ "$BUILD_TYPE" = "Nasty" ]; then
    LD_LIBRARY_PATH_ORIG=${LD_LIBRARY_PATH}
    # make sure that Nasty-MPI can be found at runtime
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${DEPLOY_PATH}/build/nastympi/lib"
    # FIXME: Building the examples does currently not work with Nasty-MPI
    export DASH_BUILDEX="OFF"
    # base delay of MPI operations in NastyMPI, in milliseconds:
    export NASTY_SLEEP_INTERVAL=0
    echo "[-> ENV    ] LD_LIBRARY_PATH: ${LD_LIBRARY_PATH}"
  fi

  echo "[-> LOG    ] $DEPLOY_PATH/build.log"
  $CMD_DEPLOY "--b=$BUILD_TYPE" -f "--i=$DEPLOY_PATH" \
              >> $DEPLOY_PATH/build.log 2>&1

  if [ "$?" = "0" ]; then
    echo "[->     OK ]"

    ### Test DASH using DART MPI backend:
    #
    echo "[-> TEST   ] Running tests on build $BUILD_TYPE (MPI)   ..."
    echo "[-> LOG    ] $DEPLOY_PATH/test_mpi.log"
    echo "[-> RUN    ] $CMD_TEST mpi $DEPLOY_PATH/bin $DEPLOY_PATH/test_mpi.log"

    $CMD_TEST mpi $DEPLOY_PATH/bin $DEPLOY_PATH/test_mpi.log
    TEST_STATUS=$?

    if [ -f $DEPLOY_PATH/test_mpi.log ] ; then
      ERROR_PATTERN="segmentation\|segfault\|terminat\|uninitialized value\|invalid read\|invalid write"
      ERROR_PATTERN_MATCHED=`grep -c -i "${ERROR_PATTERN}" $DEPLOY_PATH/test_mpi.log`
      if [ "$TEST_STATUS" = "0" ]; then
        if [ "$ERROR_PATTERN_MATCHED" = "0" ]; then
          echo "[->     OK ]"
        else
          FAILED=true
          echo "[->  ERROR ] Error pattern detected (matches: $ERROR_PATTERN_MATCHED). Check logs"
        fi
      else
        FAILED=true
        echo "[-> FAILED ] Test run failed"
      fi
    else
      FAILED=true
      echo "[->  ERROR ] No test log found"
    fi
  else
    FAILED=true
    echo "[-> FAILED ] Build failed"
  fi

  if $FAILED; then
    echo "[-> FAILED ] Integration test in $BUILD_TYPE build failed"
  else
    echo "[-> PASSED ] Build and test suite passed"
  fi

  if [ "$LD_LIBRARY_PATH_ORIG" != "" ]; then
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH_ORIG}
  fi
}

if [ $# != 0 ]; then
  for buildtype in "$@" ; do
    if $FAILED; then
      exit 127
    fi
    run_ci $buildtype
  done
else
  run_ci Release
  run_ci Minimal
  run_ci Nasty
fi

if $FAILED; then
  exit 127
fi

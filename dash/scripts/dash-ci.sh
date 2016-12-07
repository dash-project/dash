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

  echo "[-> BUILD  ] Deploying build $BUILD_TYPE to $DEPLOY_PATH ..."
  echo "[-> LOG    ] $DEPLOY_PATH/build.log"
  $CMD_DEPLOY "--b=$BUILD_TYPE" -f "--i=$DEPLOY_PATH" >> $DEPLOY_PATH/build.log 2>&1

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
      ERROR_PATTERNS=`grep -c -i "segmentation\|segfault\|terminat\|uninitialised value\|Invalid read\|Invalid write" $DEPLOY_PATH/test_mpi.log`
      if [ "$TEST_STATUS" = "0" ]; then
        if [ "$ERROR_PATTERNS" -ne "0" ]; then
          FAILED=true
          echo "[->  ERROR ] Error pattern detected. Check logs"
        else
          echo "[->     OK ]"
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
    echo "[-> FAILED ] Integration test on $BUILD_TYPE build failed"
  else
    echo "[-> PASSED ] Build and test suite passed"
  fi
}

if [ $# != 0 ]; then
  for buildtype in "$@" ; do
    if $FAILED; then
      exit -1
    fi
    run_ci $buildtype
  done
else
  run_ci Release
  run_ci Minimal
  run_ci Nasty
fi

if $FAILED; then
  exit 1
fi

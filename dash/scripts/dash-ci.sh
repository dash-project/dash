#!/bin/sh

TIMESTAMP=`date +%Y%m%d-%H%M%S`
BASEPATH=`git rev-parse --show-toplevel`
CMD_DEPLOY=$BASEPATH/dash/scripts/dash-ci-deploy.sh
CMD_TEST=$BASEPATH/dash/scripts/dash-test.sh
FAILED=false

# typeset -f module > /dev/null
# if [ $? != 0 -a -r /etc/profile.d/modules.sh ] ; then
#   source /etc/profile.d/modules.sh
#
#   module load intel
# fi

run_ci()
{
  BUILD_TYPE=${1}
  DEPLOY_PATH=$BASEPATH/build-ci/$TIMESTAMP/${BUILD_TYPE}

  mkdir -p $DEPLOY_PATH && \
    cd $DEPLOY_PATH

  echo "[ BUILD  ] Deploying build $BUILD_TYPE to $DEPLOY_PATH ..."
  echo "[ >> LOG ] $DEPLOY_PATH/build.log"
  $CMD_DEPLOY "--b=$BUILD_TYPE" -f "--i=$DEPLOY_PATH" >> $DEPLOY_PATH/build.log 2>&1

  if [ "$?" = "0" ]; then
    echo "[     OK ]"

    ### Test DASH using DART MPI backend:
    #
    echo "[ TEST   ] Running tests on build $BUILD_TYPE (MPI)   ..."
    echo "[ >> LOG ] $DEPLOY_PATH/test_mpi.log"
    if [ "$VERBOSE_CI" = "" ]; then
      $CMD_TEST mpi   $DEPLOY_PATH/bin $DEPLOY_PATH/test_mpi.log > /dev/null 2>&1
    else
      $CMD_TEST mpi   $DEPLOY_PATH/bin $DEPLOY_PATH/test_mpi.log | grep -v "LOG ="
    fi
    TEST_STATUS=$?
    ERROR_PATTERNS=`grep -c -i "segmentation\|segfault\|terminat" $DEPLOY_PATH/test_mpi.log`
    if [ "$TEST_STATUS" = "0" ]; then
      if [ "$ERROR_PATTERNS" -ne "0" ]; then
        FAILED=true
        echo "[  ERROR ] error pattern detected. Check logs"
      else
        echo "[     OK ]"
      fi
    else
      FAILED=true
      echo "[ FAILED ]"
      tail -n 100000 $DEPLOY_PATH/test_mpi.log
    fi
  else
    FAILED=true
    echo "[ FAILED ] Build failed"
    cat $DEPLOY_PATH/build.log
  fi

  if $FAILED; then
    echo "[ FAILED ] Integration test on $BUILD_TYPE build failed"
  else
    echo "[ PASSED ] Build and test suite passed"
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
  run_ci Nasty 
fi

if $FAILED; then
  exit 1
fi

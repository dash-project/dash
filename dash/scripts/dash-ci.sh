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

### Test DASH using DART SHMEM backend:
#   echo -n "[ TEST   ] Running tests on build $BUILD_TYPE (SHMEM) ..."
#   $CMD_TEST shmem $DEPLOY_PATH/bin $DEPLOY_PATH/test_shmem.log > /dev/null 2>&1
#   if [ "$?" = "0" ]; then
#     echo " OK"
#   else
#     echo " FAILED"
#     FAILED=true
#   fi
#   echo "[ <!>    ] Review the test log:"
#   echo "[ <!>    ]   $DEPLOY_PATH/test_shmem.log"

### Test DASH using DART MPI backend:
    echo "[ TEST   ] Running tests on build $BUILD_TYPE (MPI)   ..."
    $CMD_TEST mpi   $DEPLOY_PATH/bin $DEPLOY_PATH/test_mpi.log > /dev/null 2>&1
    echo "[ >> LOG ] $DEPLOY_PATH/test_mpi.log"
    if [ "$?" = "0" ]; then
      echo "[     OK ]"
    else
      FAILED=true
      echo "[ FAILED ]"
      head -n 100000 $DEPLOY_PATH/test_mpi.log
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

for buildtype in "$@" ; do
  run_ci $buildtype
done

if $FAILED; then
  exit -1
fi


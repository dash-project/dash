#!/bin/sh

BASEPATH=`git rev-parse --show-toplevel`

DEBUG_DEPLOY_PATH=$BASEPATH/build-ci/Debug
RELEASE_DEPLOY_PATH=$BASEPATH/build-ci/Release

FAILED=false

rm -Rf $DEBUG_DEPLOY_PATH && \
mkdir -p $DEBUG_DEPLOY_PATH && (cd $DEBUG_DEPLOY_PATH && \
  echo "[ BUILD  ] Deploying build type Debug to $DEBUG_DEPLOY_PATH ..." &&
  $BASEPATH/dash/scripts/dash-deploy.sh -d -f "--i=$DEBUG_DEPLOY_PATH" \
  >> build.log 2>&1 && \
  echo "[ TEST   ] Running tests on build Debug ..." &&
  $BASEPATH/dash/scripts/dash-test.sh mpi $DEBUG_DEPLOY_PATH/bin >/dev/null 2>&1 && \
  $BASEPATH/dash/scripts/dash-test.sh shmem $DEBUG_DEPLOY_PATH/bin >/dev/null 2>&1 )
if [ `find $DEBUG_DEPLOY_PATH -name 'dash-test*.log' | xargs cat | grep --count 'FAIL'` -gt 0 ]; then
  echo "[ FAIL   ] Failed tests!"
  FAILED=true
else
  echo "[ PASSED ] Build and test suite passed"
fi

rm -Rf $RELEASE_DEPLOY_PATH && \
mkdir -p $RELEASE_DEPLOY_PATH && (cd $RELEASE_DEPLOY_PATH && \
  echo "[ BUILD  ] Deploying build type Release to $RELEASE_DEPLOY_PATH ..." &&
  $BASEPATH/dash/scripts/dash-deploy.sh -r -f "--i=$RELEASE_DEPLOY_PATH" \
  >> build.log 2>&1 && \
  echo "[ TEST   ] Running tests on build Release ..." &&
  $BASEPATH/dash/scripts/dash-test.sh mpi $RELEASE_DEPLOY_PATH/bin >/dev/null 2>&1 && \
  $BASEPATH/dash/scripts/dash-test.sh shmem $RELEASE_DEPLOY_PATH/bin >/dev/null 2>&1 )
if [ `find $RELEASE_DEPLOY_PATH -name 'dash-test*.log' | xargs cat | grep --count 'FAIL'` -gt 0 ]; then
  echo "[ FAIL   ] Failed tests!"
  FAILED=true
else
  echo "[ PASSED ] Build and test suite passed"
fi

if $FAILED; then
  exit -1
fi


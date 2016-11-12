#!/bin/sh

BUILD_TYPE=$1

echo "Starting docker container for build target $BUILD_TYPE ..."

docker run -v $PWD:/opt/dash dashproject/ci:openmpi /bin/sh -c \
              "export DASH_MAX_UNITS='2'; sh dash/scripts/dash-ci.sh ${BUILD_TYPE} | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"

echo "checking logs"

cat $PWD/dash-ci.log | grep "FAILED"
if [ "$?" -eq "0" ]; then
  echo "Log contains errors"
  return 1
fi

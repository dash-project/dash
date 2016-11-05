#!/bin/sh

echo "Starting docker container ..."

docker run -v $PWD:/opt/dash dashproject/ci-testing:openmpi /bin/sh -c "export DASH_MAX_UNITS='2'; sh dash/scripts/dash-ci.sh | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"

echo "checking logs"

cat $PWD/dash-ci.log | grep "FAILED"
if [ "$?" -eq "0" ]; then
  echo "Log contains errors"
  return 1
fi

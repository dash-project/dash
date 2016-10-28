#!/bin/sh

echo "Starting docker container ..."

docker run -v $PWD:/opt/dash dash/testing /bin/sh -c "export DASH_MAX_UNITS='4'; export GTEST_TOTAL_SHARDS=${CIRCLE_NODE_TOTAL}; export GTEST_SHARD_INDEX=${CIRCLE_NODE_INDEX}; sh dash/scripts/dash-ci.sh | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"

cp ./build-ci/*/*/dash-tests-*.xml $CIRCLE_TEST_REPORTS/

echo "checking logs"

cat $PWD/dash-ci.log | grep "FAILED"
if [ "$?" -eq "0" ]; then
  echo "Log contains errors"
  return 1
fi

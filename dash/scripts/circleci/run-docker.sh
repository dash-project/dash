#!/bin/sh

echo "Starting docker container ..."

docker run -v $PWD:/opt/dash dash/testing /bin/sh -c "export DASH_MAKE_PROCS='4'; export DASH_MAX_UNITS='2'; export GTEST_TOTAL_SHARDS=${CIRCLE_NODE_TOTAL}; export GTEST_SHARD_INDEX=${CIRCLE_NODE_INDEX}; sh dash/scripts/dash-ci.sh | grep -v 'LOG =' | tee dash-ci.log 2> dash-ci.err;"

TARGETS=`ls -d ./build-ci/*/* | xargs -n1 basename`
for TARGET in $TARGETS; do
  mkdir -p $CIRCLE_TEST_REPORTS/$TARGET
  cp ./build-ci/*/$TARGET/dash-tests-*.xml $CIRCLE_TEST_REPORTS/$TARGET/
done

echo "checking logs"

cat $PWD/dash-ci.log | grep "FAILED"
if [ "$?" -eq "0" ]; then
  echo "Log contains errors"
  return 1
fi

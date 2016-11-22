#!/bin/sh

GIT_BASEPATH=`git rev-parse --show-toplevel | tr -d '\n'`
TEST_EXEC_PATH=$GIT_BASEPATH/build
LOG_PATH="$TEST_EXEC_PATH/logs"
NUNITS=$1

usage()
{
  echo "Run all test suites in single, independent test runs."
  echo ""
  echo "Usage: dash-test-all-single.sh <num units>"
  echo ""
}

if [ $# -lt 1 ]; then
  usage
  exit -1
fi

cd $TEST_EXEC_PATH || exit -1
echo "changed to path $TEST_EXEC_PATH"

mkdir -p $LOG_PATH
echo "writing logs to $LOG_PATH"

FAILED=false
for TESTSUITE in `find ../dash/test -name '*.cc' | sed "s/..\/dash\/test\/\(.\+\)\.cc/\1/"` ; do
  echo "[ START >> ] $TESTSUITE (units:$NUNITS)" ;
  mpirun -n $NUNITS ./bin/dash-test-mpi --gtest_filter="$TESTSUITE.*" 2>&1 \
    | tee "$LOG_PATH/test.$TESTSUITE.log" \
    | grep 'RUN\|PASSED\|OK\|FAILED\|ERROR'
  if [ "$?" != "0" ] ; then
    FAILED=true
  fi
done

if $FAILED ; then
  exit -1
fi

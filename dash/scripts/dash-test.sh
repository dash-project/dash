#!/bin/sh

function usage
{
  echo "Run unit tests with varying number of proceses on a"
  echo "DASH installation"
  echo ""
  echo "Usage: dash-test.sh <mpi|shmem> <bin path> [log file]"
  echo ""
  echo "... with <bin path> pointing to the directory where the"
  echo "DASH binaries have been installed, e.g. ~/opt/dash/bin"
}

if [ $# -lt 2 ]; then
  usage
  exit -1
fi

DART_IMPL="$1"
BIN_PATH="$2"
LOGFILE="$3"
RUN_CMD=""
TEST_BINARY=""

if [ "$LOGFILE" = "" ]; then
  # Use temporary log file
  LOGFILE="dash-test-${TIMESTAMP}.log"
  trap "rm $LOGFILE; exit" SIGHUP SIGINT SIGTERM
else
  echo "[        ] Writing output to $LOGFILE"
fi

if [ $DART_IMPL = "shmem" ]; then
  RUN_CMD="$BIN_PATH/dartrun-shmem"
  TEST_BINARY="$BIN_PATH/dash/test/shmem/dash-test-shmem"
elif [ $DART_IMPL = "mpi" ]; then
  RUN_CMD="mpirun"
  TEST_BINARY="$BIN_PATH/dash/test/mpi/dash-test-mpi"
else
  usage
fi

TESTS_PASSED=true
function run_suite
{
  echo "[ RUN    ] ${RUN_CMD} -n ${1} ${TEST_BINARY}" | tee -a $LOGFILE
  $RUN_CMD -n $1 $TEST_BINARY 2>&1 | sed 's/\x1b\[[0-9;]*m//g' | \
    tee -a $LOGFILE | grep 'FAIL'
  if [ "$?" = "0" ]; then
    echo "[ OK     ] Test run passed" | tee -a $LOGFILE
  else
    FAIL_COUNT=`grep --count 'FAIL' $LOGFILE`
    TESTS_PASSED=false
    echo "[ FAIL   ] $FAIL_COUNT failed tests" | tee -a $LOGFILE
  fi
}

run_suite 1
run_suite 2
run_suite 3
run_suite 4
run_suite 7
run_suite 8
run_suite 11
run_suite 12

if $TESTS_PASSED; then
  exit 0
else
  exit -1
fi


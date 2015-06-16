#!/bin/sh

function usage
{
  echo "Usage: dash-test.sh <mpi|shmem> <bin path>"
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
RUN_CMD=""
TEST_BINARY=""

TIMESTAMP=$(date +%Y-%m-%d--%H%M%S)
LOGFILE="dash-test-$DART_IMPL--$TIMESTAMP.log"

echo "Writing output to $LOGFILE"
sleep 3

if [ $DART_IMPL = "shmem" ]; then
  RUN_CMD="$BIN_PATH/dartrun-shmem"
  TEST_BINARY="$BIN_PATH/dash/test/shmem/dash-test-shmem"
elif [ $DART_IMPL = "mpi" ]; then
  RUN_CMD="mpirun"
  TEST_BINARY="$BIN_PATH/dash/test/mpi/dash-test-mpi"
else
  usage
fi

function run_suite
{
  echo "===================================" >> $LOGFILE
  echo "Running test suite with ${1} units" >> $LOGFILE
  $RUN_CMD -n $1 $TEST_BINARY 2>&1 | tee -a $LOGFILE | grep Failure
  echo "===================================" >> $LOGFILE
}

run_suite 1
run_suite 2
run_suite 3
run_suite 4
run_suite 7
run_suite 8
run_suite 20
run_suite 21


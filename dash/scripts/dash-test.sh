#!/bin/sh

usage()
{
  echo "Run unit tests with varying number of proceses on a"
  echo "DASH installation"
  echo ""
  echo "Usage: dash-test.sh <mpi|shmem> <bin path> [log file]"
  echo ""
  echo "... with <bin path> pointing to the directory where the"
  echo "DASH binaries have been installed, e.g. ~/opt/dash/bin"
  echo ""
}

nocolor()
{
  sed 's/\x1b\[[0-9;]*m//g'
}

if [ $# -lt 2 ]; then
  usage
  exit -1
fi

TIMESTAMP=`date +%Y%m%d-%H%M%S`
NCORES=`cat /proc/cpuinfo | grep -c 'core id'`
DART_IMPL="$1"
BIN_PATH="$2"
LOGFILE="$3"
BIND_CMD=""
TEST_BINARY=""

if [ "$LOGFILE" = "" ]; then
  # Use temporary log file
  LOGFILE="dash-test-${TIMESTAMP}.log"
  trap "rm $LOGFILE; exit" SIGHUP SIGINT SIGTERM
else
  echo "[[        ]] Writing output to $LOGFILE"
fi

if [ $DART_IMPL = "shmem" ]; then
  RUN_CMD="$BIN_PATH/dartrun-shmem"
  TEST_BINARY="${EXEC_WRAP} $BIN_PATH/dash/test/shmem/dash-test-shmem"
elif [ $DART_IMPL = "mpi" ]; then
#  if (mpirun --help | grep -ic "open\(.\)\?mpi" >/dev/null 2>&1) ; then
#   MPI_EXEC_FLAGS="--map-by core ${MPI_EXEC_FLAGS}"
#  fi
  RUN_CMD="${EXEC_PREFIX} mpirun ${MPI_EXEC_FLAGS}"
  TEST_BINARY="${EXEC_WRAP} $BIN_PATH/dash/test/mpi/dash-test-mpi"
else
  usage
fi

# Number of failed tests in total
TOTAL_FAIL_COUNT=0
TESTS_PASSED=true
run_suite()
{
  NCORES=`cat /proc/cpuinfo | grep -c 'core id'`
  NUNITS=$1
  if ! [ "$DASH_MAX_UNITS" = "" ]; then
    NCORES=$DASH_MAX_UNITS
  fi
  if [ $NCORES -lt $NUNITS ]; then
    return 0
  fi
  BIND_CMD=""
  MAX_RANK=$((NUNITS - 1))
# if [ `which numactl` ]; then
#   BIND_CMD="numactl --physcpubind=0-${MAX_RANK}"
# fi
  export GTEST_OUTPUT="xml:dash-tests-${NUNITS}.xml"
  echo "[[== START ====================================================]]" | \
    tee -a $LOGFILE
  echo "[[ RUN    ]] ${RUN_CMD} -n ${NUNITS} ${BIND_CMD} ${TEST_BINARY}" | \
    tee -a $LOGFILE
  eval $RUN_CMD -n $1 $BIND_CMD $TEST_BINARY 2>&1 | nocolor | \
    tee -a $LOGFILE | grep -v 'LOG' | grep -v '^#'
  TEST_RET=$?
  # Cannot use exit code as dartrun-shmem seems to always return 0
  NEW_FAIL_COUNT=`grep --count 'FAILED TEST' $LOGFILE`
  # Failure is logged by every unit, divide reported failures by number of
  # units:
  NEW_FAIL_COUNT=$(($NEW_FAIL_COUNT / $NUNITS))
  # Number of failed tests in this run
  THIS_FAIL_COUNT=$(($NEW_FAIL_COUNT-$TOTAL_FAIL_COUNT))
  TOTAL_FAIL_COUNT=$NEW_FAIL_COUNT
  if [ "$THIS_FAIL_COUNT" = "0" ]; then
    echo "[[     OK ]] Tests passed, returned ${TEST_RET}" | \
      tee -a $LOGFILE
  else
    TESTS_PASSED=false
    echo "[[   FAIL ]] $THIS_FAIL_COUNT failed tests, returned ${TEST_RET}" | \
      tee -a $LOGFILE
  fi
  sleep 3
  echo "[[== FINISHED =================================================]]" | \
    tee -a $LOGFILE
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
  exit 127
fi


#!/bin/sh

TIMESTAMP=`date +%Y%m%d-%H%M%S`
NCORES=`cat /proc/cpuinfo | grep -c 'core id'`
DART_IMPL="$1"
BIN_PATH="$2"
LOGFILE="$3"
BIND_CMD=""
TEST_BINARY=""

usage()
{
  echo "Run all test suites in single, independent test runs"
  echo "with varying number of proceses on a DASH installation"
  echo ""
  echo "Usage: dash-test-all-single.sh [mpi|shmem] [bin path] [log file]"
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

if [ "$LOGFILE" = "" ]; then
  # Use temporary log file
  LOGFILE="dash-test-${TIMESTAMP}.log"
  trap "rm $LOGFILE; exit" SIGHUP SIGINT SIGTERM
fi

echo "[[ LOG    ]] Writing output to $LOGFILE"

if [ $DART_IMPL = "shmem" ]; then
  RUN_CMD="$BIN_PATH/dartrun-shmem"
  TEST_BINARY="${EXEC_WRAP} $BIN_PATH/dash/test/shmem/dash-test-shmem"
elif [ $DART_IMPL = "mpi" ]; then
# if (mpirun --help | grep -ic "open\(.\)\?mpi" >/dev/null 2>&1) ; then
# fi
  MPI_EXEC_FLAGS="-map-by core ${MPI_EXEC_FLAGS}"
  RUN_CMD="${EXEC_PREFIX} mpirun ${MPI_EXEC_FLAGS}"
  TEST_BINARY="${EXEC_WRAP} $BIN_PATH/dash/test/mpi/dash-test-mpi"
else
  usage
  exit -1
fi

if [ "$GTEST_FILTER" = "" ] ; then
  OUTPUT=`$RUN_CMD -n 1 $TEST_BINARY --gtest_list_tests`
else
  OUTPUT=`$RUN_CMD -n 1 $TEST_BINARY --gtest_list_tests --gtest_filter=$GTEST_FILTER`
fi

ret=$?
if [ $ret -ne 0 ] ; then
  echo "[[   FAIL ]] [ $(date +%Y%m%d-%H%M%S) ]:"
  echo "$OUTPUT"
  exit  $ret
fi

TEST_SUITES=$(echo "$OUTPUT" | grep -v '^\s' | grep -v '^#')

# Number of failed tests in total
TOTAL_FAIL_COUNT=0
TESTS_PASSED=true


run_suite()
{
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
  TESTREPORT_XML="dash-tests-${NUNITS}.xml"
  for TESTSUITE in $TEST_SUITES ; do
    TESTSUITE_XML="dash-tests-${TESTSUITE}${NUNITS}.xml"
    TESTSUITE_LOG="test.${TESTSUITE}${NUNITS}.log"
    TEST_PATTERN="${TESTSUITE}*"

    export GTEST_OUTPUT="xml:$TESTSUITE_XML"
    if [ "$GTEST_FILTER" != "" ] ; then
      TEST_PATTERN="${TEST_PATTERN}:${GTEST_FILTER}"
    fi
    echo "[[ SUITE  ]] [ $(date +%Y%m%d-%H%M%S) ] ${TESTSUITE}*" \
         | tee -a $LOGFILE
    echo "[[ RUN    ]] [ $(date +%Y%m%d-%H%M%S) ] $RUN_CMD -n $NUNITS $BIND_CMD $TEST_BINARY --gtest_filter='${TEST_PATTERN}'" \
         | tee -a $LOGFILE
    eval "$RUN_CMD -n $NUNITS $BIND_CMD $TEST_BINARY --gtest_filter='$TEST_PATTERN'" 2>&1 \
         | nocolor \
         | tee -a $LOGFILE \
         | grep -v 'LOG' | grep -v '^#' \
         | tee $TESTSUITE_LOG

    cat $TESTSUITE_XML >> $TESTREPORT_XML

    TEST_RET="$?"

    TESTSUITE_FAIL_COUNT=`grep --count 'FAILED TEST' $TESTSUITE_LOG`
    TESTSUITE_PASS_COUNT=`grep --count "PASSED" $TESTSUITE_LOG`

    if [ "$TESTSUITE_PASS_COUNT" -gt 0 ] && [ "$TESTSUITE_FAIL_COUNT" -eq "0" ] && [ "$TEST_RET" -eq "0" ] ; then
      echo "[[     OK ]] [ $(date +%Y%m%d-%H%M%S) ] Tests passed, returned ${TEST_RET}" | \
        tee -a $LOGFILE
    else
      TESTS_PASSED=false
      echo "[[   FAIL ]] [ $(date +%Y%m%d-%H%M%S) ] $TESTSUITE_FAIL_COUNT failed, returned ${TEST_RET}, completed: ${TESTSUITE_PASS_COUNT}" | \
        tee -a $LOGFILE
    fi
  done
}

run_suite 1
run_suite 2
run_suite 3
run_suite 4
run_suite 6
run_suite 7
run_suite 8
run_suite 11
run_suite 12

if $TESTS_PASSED; then
  exit 0
else
  exit 127
fi


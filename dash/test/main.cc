#include "TestGlobals.h"
#include "TestBase.h"
#include "TestPrinter.h"

#include <gtest/gtest.h>

#include <libdash.h>
#include <iostream>

#ifdef MPI_IMPL_ID
  #include <mpi.h>
  #define MPI_SUPPORT
#endif

using ::testing::UnitTest;
using ::testing::TestEventListeners;


int main(int argc, char * argv[])
{
  char hostname[100];
  int team_myid = -1;
  int team_size = -1;
  TESTENV.argc = argc;
  TESTENV.argv = argv;

  gethostname(hostname, 100);
  std::string host(hostname);

  // Init MPI
  #ifdef MPI_SUPPORT
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &team_myid);
  MPI_Comm_size(MPI_COMM_WORLD, &team_size);

  // only unit 0 writes xml file
  if(team_myid != 0){
    ::testing::GTEST_FLAG(output) = "";
  }

  #endif
  // Init GoogleTest (strips gtest arguments from argv)
  ::testing::InitGoogleTest(&argc, argv);
  #ifdef MPI_SUPPORT
  MPI_Barrier(MPI_COMM_WORLD);
  #endif

  std::cout << "#### "
            << "Starting test on unit " << team_myid << " "
            << "(" << host << " PID: " << getpid() << ")"
            << std::endl;
  #ifdef MPI_SUPPORT
  MPI_Barrier(MPI_COMM_WORLD);
  #endif

  sleep(1);

  #ifdef MPI_SUPPORT
  // Parallel Test Printer only available for MPI
  // Change Test Printer
  UnitTest& unit_test = *UnitTest::GetInstance();
  TestEventListeners& listeners = unit_test.listeners();

  delete listeners.Release(listeners.default_result_printer());

  listeners.Append(new TestPrinter);
  #endif

  // Run Tests
  int ret = RUN_ALL_TESTS();

  #ifdef MPI_SUPPORT
  if (dash::is_initialized()) {
    dash::finalize();
  }
  MPI_Finalize();
  #endif
  return ret;
}

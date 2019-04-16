#include "TestGlobals.h"
#include "TestBase.h"
#include "TestPrinter.h"

#include <gtest/gtest.h>

#include <libdash.h>
#include <iostream>

#ifdef DASH_MPI_IMPL_ID
  #include <mpi.h>
  #define MPI_SUPPORT
#endif

#ifdef DASH_GASPI_IMPL_ID
  #include <GASPI.h>
  #define GASPI_SUPPORT
#endif

using ::testing::UnitTest;
using ::testing::TestEventListeners;

int     TESTENV::argc;
char ** TESTENV::argv;

int main(int argc, char * argv[])
{
  //printf("Bginning of test_main\n");
  char hostname[100];

#ifdef GASPI_SUPPORT
  gaspi_rank_t team_myid = 0;
  gaspi_rank_t team_size = 0;
#else
  int  team_myid = -1;
  int  team_size = -1;
#endif
  TESTENV::argc = argc;
  TESTENV::argv = argv;

  gethostname(hostname, 100);
  std::string host(hostname);

  // Init MPI
#ifdef MPI_SUPPORT
#ifdef DASH_ENABLE_THREADSUPPORT
  int thread_required = MPI_THREAD_MULTIPLE;
  int thread_provided; // ignored here
  MPI_Init_thread(&argc, &argv, thread_required, &thread_provided);
#else
  MPI_Init(&argc, &argv);
#endif // DASH_ENABLE_THREADSUPPORT
  MPI_Comm_rank(MPI_COMM_WORLD, &team_myid);
  MPI_Comm_size(MPI_COMM_WORLD, &team_size);

  // only unit 0 writes xml file
  if(team_myid != 0){
    ::testing::GTEST_FLAG(output) = "";
  }

#endif

// Init GASPI
#ifdef GASPI_SUPPORT

gaspi_proc_init(GASPI_BLOCK);

gaspi_proc_rank(&team_myid);
gaspi_proc_num(&team_size);

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

#ifdef GASPI_SUPPORT
  gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);
#endif

  sleep(1);

  // Parallel Test Printer only available for MPI
  // Change Test Printer
  UnitTest& unit_test = *UnitTest::GetInstance();
  TestEventListeners& listeners = unit_test.listeners();

  delete listeners.Release(listeners.default_result_printer());

  listeners.Append(new TestPrinter);

  // Run Tests
  int ret = RUN_ALL_TESTS();

#ifdef MPI_SUPPORT
  if (dash::is_initialized()) {
    dash::finalize();
  }
  MPI_Finalize();
#endif

//Gaspi finalize
#ifdef GASPI_SUPPORT
  if(dash::is_initialized()) {
    dash::finalize();
  }
  gaspi_proc_term(GASPI_BLOCK);
#endif

return ret;
}

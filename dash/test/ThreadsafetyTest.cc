
#include "ThreadsafetyTest.h"

#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Array.h>
#include <dash/Distribution.h>
#include <dash/Dimensional.h>
#include <dash/util/TeamLocality.h>

#include <mpi.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

TEST_F(ThreadsafetyTest, ThreadInit) {
  int mpi_thread;
  MPI_Query_thread(&mpi_thread);

  EXPECT_TRUE_U((mpi_thread == MPI_THREAD_MULTIPLE) == dash::is_multithreaded() );
}

TEST_F(ThreadsafetyTest, ConcurrentPut) {

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

#if !defined(_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else
  int num_threads;
#pragma omp parallel
  {
#pragma omp master
  {
    num_threads = omp_get_num_threads();
  }
  }

  dash::Array<int> array(dash::size() * num_threads);
  dash::default_index_t idx = ((dash::myid() + 1) % dash::size()) * num_threads;
#pragma omp parallel
  {
    int  thread_id = omp_get_thread_num();
    for (int i = 0; i < 100; ++i) {
      array[idx + thread_id] = thread_id;
    }
  }

  dash::barrier();

//  for (int i = 0; i < num_threads; ++i) {
//    std::cout << "[" << dash::myid() << "] array[" << i << "] = " << (int)array[i] << std::endl;
//  }

  for (int i = 0; i < num_threads; ++i) {
    ASSERT_EQ_U(array.local[i], i);
  }
#endif

}

TEST_F(ThreadsafetyTest, ConcurrentAlloc) {

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

#if !defined(_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else
  int num_threads;
#pragma omp parallel
#pragma omp master
  {
    num_threads = omp_get_num_threads();
  }

  dash::Team& team_all   = dash::Team::All();
  dash::Team& team_split = team_all.split(2);
  ASSERT_GT_U(team_all.size(), 0);
  ASSERT_GT_U(team_split.size(), 0);
  dash::Array<int> arr_all;
  dash::Array<int> arr_split;

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    for (int i = 0; i < 100; ++i) {
      // thread 0: contribute to allocation on team_all
      if (thread_id == 0) {
        if (i) {
          arr_all.deallocate();
          ASSERT_EQ_U(arr_all.size(), 0);
        }
        arr_all.allocate(team_all.size(), dash::DistributionSpec<1>(), team_all);
        ASSERT_EQ_U(arr_all.size(), team_all.size());
        arr_all[(team_all.myid() + 1) % team_all.size()] = 0;
      } else if(thread_id == 1) {
        if (i) {
          arr_split.deallocate();
          ASSERT_EQ_U(arr_split.size(), 0);
        }
        arr_split.allocate(team_split.size(), dash::DistributionSpec<1>(), team_split);
        ASSERT_EQ_U(arr_split.size(), team_split.size());
        arr_split[(team_split.myid() + 1) % team_split.size()] = 1;
      }
    }

#pragma omp barrier
#pragma omp master
    {
      arr_all.barrier();
      arr_split.barrier();
      ASSERT_EQ_U(arr_all.local[0], 0);
      ASSERT_EQ_U(arr_split.local[0], 1);
    }
#pragma omp barrier
  }

#endif



}

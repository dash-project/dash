
#include "ThreadsafetyTest.h"

#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Array.h>
#include <dash/Distribution.h>
#include <dash/Dimensional.h>
#include <dash/allocator/DynamicAllocator.h>
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

  using elem_t  = int;
  using array_t = dash::Array<elem_t>;

  constexpr size_t elem_per_thread = 100;

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

  size_t size = dash::size() * num_threads * elem_per_thread;
  array_t src(size);
  array_t dst(size);

#pragma omp parallel
  {
    int  thread_id = omp_get_thread_num();
    array_t::index_type base_idx = thread_id * elem_per_thread;
    for (size_t i = 0; i < elem_per_thread; ++i) {
      src.local[base_idx + i] = thread_id;
    }
  }

  src.barrier();

#pragma omp parallel
  {
    int  thread_id = omp_get_thread_num();
    array_t::index_type src_idx =   dash::myid()
                                        * (elem_per_thread * num_threads)
                                        + (elem_per_thread * thread_id);
    array_t::index_type dst_idx = ((dash::myid() + 1) % dash::size())
                                    * (elem_per_thread * num_threads)
                                    + (elem_per_thread * thread_id);
    for (size_t i = 0; i < elem_per_thread; ++i) {
      dst[dst_idx + i] = src[src_idx + i];
    }
  }

  dash::barrier();

//  for (int i = 0; i < num_threads; ++i) {
//    std::cout << "[" << dash::myid() << "] array[" << i << "] = " << (int)array[i] << std::endl;
//  }
  size_t pos = 0;
  for (int i = 0; i < num_threads; ++i) {
    for (size_t j = 0; j  < elem_per_thread; ++j) {
      ASSERT_EQ_U(dst.local[pos++], i);
    }
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

  using elem_t  = int;
  using array_t = dash::Array<elem_t>;
  constexpr size_t elem_per_thread = 100;

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
  array_t arr_all;
  array_t arr_split;

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
    array_t    *arr  = (thread_id == 0) ? &arr_all : &arr_split;
    for (int i = 0; i < 100; ++i) {
      // thread 0: contribute to allocation on team_all
      if (i) {
        arr->deallocate();
      }
      ASSERT_EQ_U(arr->size(), 0);
      arr->allocate(elem_per_thread * team->size(),
              dash::DistributionSpec<1>(), *team);
      ASSERT_EQ_U(arr->size(), elem_per_thread * team->size());
#pragma omp barrier
      ASSERT_NE_U(
          arr_all[0].dart_gptr().segid,
          arr_split[0].dart_gptr().segid);
#pragma omp barrier
      array_t::index_type base =
          ((team->myid() + 1) % team->size()) * elem_per_thread;
      for (size_t j = 0; j < elem_per_thread; ++j) {
        (*arr)[base + j] = thread_id;
      }
//      if (thread_id == 0) {
//        if (i) {
//          arr_all.deallocate();
//          ASSERT_EQ_U(arr_all.size(), 0);
//        }
//        arr_all.allocate(elem_per_thread * team_all.size(),
//                dash::DistributionSpec<1>(), team_all);
//        ASSERT_EQ_U(arr_all.size(), elem_per_thread * team_all.size());
//        array_t::index_type base = (team_all.myid() + 1) % team_all.size();
//        for (size_t j = 0; j < elem_per_thread; ++j) {
//          arr_all[base + j] = 0;
//        }
//      } else if(thread_id == 1) {
//        if (i) {
//          arr_split.deallocate();
//          ASSERT_EQ_U(arr_split.size(), 0);
//        }
//        arr_split.allocate(elem_per_thread * team_split.size(),
//                dash::DistributionSpec<1>(), team_split);
//        ASSERT_EQ_U(arr_split.size(), elem_per_thread * team_split.size());
//        array_t::index_type base = (team_split.myid() + 1) % team_split.size();
//        for (size_t j = 0; j < elem_per_thread; ++j) {
//          arr_split[base + j] = 1;
//        }
//      }
    }

#pragma omp barrier
#pragma omp master
    {
      arr_all.barrier();
      arr_split.barrier();
      for (size_t i = 0; i < elem_per_thread; ++i) {
        ASSERT_EQ_U(arr_all.local[i], 0);
        ASSERT_EQ_U(arr_split.local[i], 1);
      }
    }
#pragma omp barrier
  }

#endif //!defined(_OPENMP)
}

TEST_F(ThreadsafetyTest, ConcurrentAttach) {

  using elem_t = int;
  using allocator_t = dash::allocator::DynamicAllocator<elem_t>;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  constexpr size_t elem_per_thread = 100;

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

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
    for (int i = 0; i < 1; ++i) {
      allocator_t allocator(*team);
      elem_t *vals = allocator.allocate_local(elem_per_thread);
      for (size_t j = 0; j < elem_per_thread; ++j) {
        vals[j] = thread_id;
      }
      std::cout << "vals: " << vals << std::endl;
      dart_gptr_t gptr = allocator.attach(vals, elem_per_thread);
      ASSERT_NE_U(DART_GPTR_NULL, gptr);
      ASSERT_LT_U(gptr.segid, 0); // attached memory has segment ID < 0
      elem_t check[elem_per_thread];
      dart_gptr_t gptr_r = gptr;
      gptr_r.unitid = team->global_id(
          dash::team_unit_t((team->myid() + 1) % team->size()));
      dart_storage_t ds = dash::dart_storage<elem_t>(elem_per_thread);
      ASSERT_EQ_U(
        dart_get_blocking(check, gptr_r, ds.nelem, ds.dtype),
        DART_OK);

      team->barrier();

      for (size_t j = 0; j < elem_per_thread; ++j) {
        ASSERT_EQ_U(check[j], thread_id);
      }
      team->barrier();

      allocator.deallocate(gptr);
    }
#pragma omp barrier
  }
#endif //!defined(_OPENMP)
}

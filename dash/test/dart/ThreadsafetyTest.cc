
#if defined(DASH_ENABLE_THREADSUPPORT)

#include "ThreadsafetyTest.h"

#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Array.h>
#include <dash/Distribution.h>
#include <dash/Algorithm.h>
#include <dash/Dimensional.h>
#include <dash/allocator/EpochSynchronizedAllocator.h>
#include <dash/util/TeamLocality.h>

#include <mpi.h>

#if defined(DASH_ENABLE_OPENMP)
#include <omp.h>
#endif

static constexpr int    thread_iterations = 1;
static constexpr size_t elem_per_thread = 10;

TEST_F(ThreadsafetyTest, ThreadInit) {
  int mpi_thread;
  MPI_Query_thread(&mpi_thread);

  EXPECT_TRUE_U((mpi_thread == MPI_THREAD_MULTIPLE) == dash::is_multithreaded() );
}

TEST_F(ThreadsafetyTest, ConcurrentPutGet) {

  using elem_t  = int;
  using array_t = dash::Array<elem_t>;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 2) {
    SKIP_TEST_MSG("requires at least 2 units");
  }


#if !defined (DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else

  size_t size = dash::size() * _num_threads * elem_per_thread;
  array_t src(size);
  array_t dst(size);

#pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    array_t::index_type base_idx = thread_id * elem_per_thread;
    for (size_t i = 0; i < elem_per_thread; ++i) {
      LOG_MESSAGE("src.local[%i] <= %i",
          static_cast<int>(base_idx + i),
          thread_id);
      src.local[base_idx + i] = thread_id;
    }
  }

  src.barrier();

#pragma omp parallel
  {
    int thread_id = omp_get_thread_num();
    array_t::index_type src_idx =   dash::myid()
                                        * (elem_per_thread * _num_threads)
                                        + (elem_per_thread * thread_id);
    array_t::index_type dst_idx = ((dash::myid() + 1) % dash::size())
                                        * (elem_per_thread * _num_threads)
                                        + (elem_per_thread * thread_id);
    for (size_t i = 0; i < elem_per_thread; ++i) {
      LOG_MESSAGE("dst[%i] <= src[%i]",
          static_cast<int>(dst_idx + i),
          static_cast<int>(src_idx + i));
      dst[dst_idx + i] = src[src_idx + i];
    }
  }

  dash::barrier();

  size_t pos = 0;
  for (int i = 0; i < _num_threads; ++i) {
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

#if !defined(DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else

  dash::Team& team_all   = dash::Team::All();
  dash::Team& team_split = team_all.split(2);
  ASSERT_GT_U(team_all.size(), 0);
  ASSERT_GT_U(team_split.size(), 0);
  array_t arr_all;
  array_t arr_split;

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    // thread 0: contribute to allocation on team_all
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
    array_t    *arr  = (thread_id == 0) ? &arr_all : &arr_split;
    for (int i = 0; i < thread_iterations; ++i) {
#pragma omp barrier
      if (i) {
        arr->deallocate();
      }
      ASSERT_EQ_U(arr->size(), 0);
      arr->allocate(elem_per_thread * team->size(),
              dash::DistributionSpec<1>(), *team);
      ASSERT_EQ_U(arr->size(), elem_per_thread * team->size());
#pragma omp barrier
      // segment IDs should be equal since
      // every team has it's own counter
      ASSERT_EQ_U(
          arr_all[0].dart_gptr().segid,
          arr_split[0].dart_gptr().segid);
#pragma omp barrier
      array_t::index_type base =
          ((team->myid() + 1) % team->size()) * elem_per_thread;
      for (size_t j = 0; j < elem_per_thread; ++j) {
        (*arr)[base + j] = thread_id;
      }
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

#endif //!defined(DASH_ENABLE_OPENMP)
}

TEST_F(ThreadsafetyTest, ConcurrentAttach) {

  using elem_t = int;
  using allocator_t = dash::allocator::EpochSynchronizedAllocator<elem_t>;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

#if !defined(DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else

  dash::Team& team_all   = dash::Team::All();
  dash::Team& team_split = team_all.split(2);
  ASSERT_GT_U(team_all.size(), 0);
  ASSERT_GT_U(team_split.size(), 0);

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
    for (int i = 0; i < thread_iterations; ++i) {
#pragma omp barrier
      allocator_t allocator(*team);
      elem_t *vals = allocator.allocate_local(elem_per_thread);
      for (size_t j = 0; j < elem_per_thread; ++j) {
        vals[j] = thread_id;
      }
      dart_gptr_t gptr = allocator.attach(vals, elem_per_thread);
      ASSERT_NE_U(DART_GPTR_NULL, gptr);
      ASSERT_LT_U(gptr.segid, 0); // attached memory has segment ID < 0
      elem_t check[elem_per_thread];
      dart_gptr_t gptr_r = gptr;
      gptr_r.unitid = (team->myid() + 1) % team->size();
      dash::dart_storage<elem_t> ds(elem_per_thread);
      ASSERT_EQ_U(
        dart_get_blocking(check, gptr_r, ds.nelem, ds.dtype, ds.dtype),
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
#endif //!defined(DASH_ENABLE_OPENMP)
}


TEST_F(ThreadsafetyTest, ConcurrentMemAlloc) {

  using elem_t    = int;
  using pointer_t = dash::GlobPtr< elem_t, dash::GlobUnitMem<elem_t> >;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  static constexpr size_t elem_per_thread = 10;

#if !defined(DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else


  dash::Team& team_all   = dash::Team::All();
  dash::Team& team_split = team_all.split(2);
  ASSERT_GT_U(team_all.size(), 0);
  ASSERT_GT_U(team_split.size(), 0);

  pointer_t ptr[_num_threads];

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
    dash::Array<pointer_t> arr;
    arr.allocate(team->size(),
                 dash::DistributionSpec<1>(), *team);

    for (int i = 0; i < thread_iterations; ++i) {
#pragma omp barrier
      ptr[thread_id] = dash::memalloc<elem_t>(elem_per_thread);
#pragma omp barrier
#pragma omp master
      {
        ASSERT_NE_U(ptr[0], ptr[1]);
      }
#pragma omp barrier
      arr.local[0] = ptr[thread_id];
      arr.barrier();
      pointer_t rptr = arr[((team->myid() + 1) % team->size())];
      for (size_t i = 0; i < elem_per_thread; ++i) {
        rptr[i] = thread_id;
      }
      arr.barrier();
      ASSERT_EQ_U(static_cast<elem_t>(ptr[thread_id][0]), thread_id);
      arr.barrier();
      dash::memfree(ptr[thread_id]);
    }
  }
#endif //!defined(DASH_ENABLE_OPENMP)
}


TEST_F(ThreadsafetyTest, ConcurrentAlgorithm) {

  using elem_t = int;
  using array_t = dash::Array<elem_t>;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  static constexpr size_t elem_per_thread = 10;

#if !defined(DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else

  dash::Team& team_all   = dash::Team::All();
  dash::Team& team_split = team_all.split(2);
  ASSERT_GT_U(team_all.size(), 0);
  ASSERT_GT_U(team_split.size(), 0);

#pragma omp parallel num_threads(2)
  {
    int thread_id = omp_get_thread_num();
    dash::Team *team = (thread_id == 0) ? &team_all : &team_split;
#pragma omp critical
    std::cout << "Thread " << thread_id << " has team " << team->dart_id() << std::endl;
    size_t num_elem = team->size() * elem_per_thread;
    array_t arr(num_elem, *team);
    elem_t *vals = new elem_t[num_elem];
    for (int i = 0; i < thread_iterations; ++i) {
#pragma omp barrier
      dash::fill(arr.begin(), arr.end(), thread_id);
      arr.barrier();

#pragma omp critical
      if (team->myid() == 0) {
        std::cout << "Thread " << thread_id << ": ";
        for (int i = 0; i < num_elem; i++) {
          std::cout << " " << (elem_t)arr[i];
        }
        std::cout << std::endl;
      }
#pragma omp barrier
      arr.barrier();

      std::function<void(elem_t &)> f = [=](const elem_t& val){
        ASSERT_EQ_U(thread_id, val);
      };
      dash::for_each(arr.begin(), arr.end(), f);


//      elem_t acc = dash::accumulate(arr.begin(), arr.end(), 0);
//      // TODO: dash::accumulate is still broken
//      if (team->myid() == 0) {
//        ASSERT_EQ_U(num_elem * thread_id, acc);
//      }
//      arr.barrier();


      dash::copy(arr.begin(), arr.end(), vals);
      ASSERT_EQ_U(vals[team->myid() * elem_per_thread], thread_id);

      std::function<elem_t(void)> g = [=](){
        return (thread_id + 1) * (team->myid() + 1);
      };
      dash::generate(arr.begin(), arr.end(), g);
      // wait here because dash::generate does not block
      arr.barrier();

#pragma omp critical
      if (team->myid() == 0) {
        std::cout << "Thread " << thread_id << ": ";
        for (int i = 0; i < num_elem; i++) {
          std::cout << " " << (elem_t)arr[i];
        }
        std::cout << std::endl;
      }
#pragma omp barrier
      elem_t min = *(dash::min_element(arr.begin(), arr.end()));
      ASSERT_EQ_U((thread_id + 1), min);
      elem_t max = *(dash::max_element(arr.begin(), arr.end()));
      ASSERT_EQ_U((thread_id + 1) * team->size(), max);
      arr.barrier();
    }
    delete[] vals;
  }
#endif // !defined(DASH_ENABLE_OPENMP)
}

#endif // DASH_ENABLE_THREADSUPPORT

/**
 * Local copy benchmark for various containers
 *
 * author(s): Felix Moessbauer, LMU Munich */
/* @DASH_HEADER@ */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <math.h>
#include <unistd.h>

using std::cout;
using std::endl;
using std::vector;

typedef int     ElementType;
typedef int64_t index_t;
typedef dash::Array<
          ElementType,
          index_t,
          dash::CSRPattern<1, dash::ROW_MAJOR, index_t>
        > Array_t;
typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

#define DASH_PRINT_MASTER(expr) \
  do { \
    if (dash::myid() == 0) { \
      std::cout << "" << expr << std::endl; \
    } \
  } while(0)

double copy_all_local(size_t size, bool parallel);
double copy_no_local(size_t size, bool parallel);

int main(int argc, char** argv)
{
  double kps_al;
  double kps_nl;
  double mem_rank;
  double mem_glob;
  size_t num_iterations = 10;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  if (dash::myid() == 0) {
    cout << "Local copy benchmark"
         << endl;
    cout << "Timer: " << Timer::TimerName()
         << endl;
    cout << std::setw(14) << "size";
    cout << std::setw(14) << "all local";
    cout << std::setw(14) << "no local";
    cout << std::setw(8)  << " ";
    cout << std::setw(14) << "mem/rank";
    cout << std::setw(14) << "mem/glob";
    cout << endl;
  }

  /// 1 GB of elements in total, distributed to dash::size() units:
  size_t size_base = (static_cast<size_t>(std::pow(2, 30)) /
                        sizeof(ElementType)) *
                     num_iterations;

  for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
    size_t size = (iteration + 1) * size_base;

    DASH_LOG_DEBUG("main", "START copy_all_local", "size: ", size_exp);
    kps_al    = copy_all_local(size, false);
    dash::barrier();

    sleep(1);
    DASH_LOG_DEBUG("main", "DONE  copy_all_local", "size: ", size_exp);

    DASH_LOG_DEBUG("main", "START copy_no_local", "size: ", size_exp);
    kps_nl    = copy_no_local(size, false);
    dash::barrier();
    DASH_LOG_DEBUG("main", "DONE  copy_no_local", "size: ", size_exp);

    mem_glob  = ((sizeof(ElementType) * size) / 1024) / 1024;
    mem_rank  = mem_glob / dash::size();

    DASH_PRINT_MASTER(
      std::setw(14) << size <<
      std::setprecision(5) << std::setw(14) << kps_al <<
      std::setprecision(5) << std::setw(14) << kps_nl <<
      std::setw(8)  << "MKeys/s" <<
      std::setw(10) << mem_rank << " MiB" <<
      std::setw(10) << mem_glob << " MiB");
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_all_local(size_t size, bool parallel)
{
  Array_t global_array(size, dash::BLOCKED);

  auto   l_start_idx = global_array.pattern().lbegin();
  auto   l_end_idx   = global_array.pattern().lend();
  size_t local_size  = l_end_idx - l_start_idx;
  auto   timer_start = Timer::Now();
  double elapsed     = 0;

  DASH_LOG_DEBUG("copy_all_local()",
                 "size:",   size,
                 "l_idcs:", l_start_idx, "-", l_end_idx,
                 "l_size:", local_size);

  dash::barrier();

  if (dash::myid() == 0 && !parallel) {
    ElementType * local_array = new ElementType[local_size];
    timer_start = Timer::Now();
    ElementType * copy_lend = dash::copy(global_array.begin() + l_start_idx,
                                         global_array.begin() + l_end_idx,
                                         local_array);
    elapsed = Timer::ElapsedSince(timer_start);
    DASH_LOG_DEBUG("copy_all_local()",
                   "l_start_idx:",     l_start_idx,
                   "l_end_idx:",       l_end_idx,
                   "copied elements:", (copy_lend - local_array),
                   "local size:",      local_size,
                   "elapsed us:",      elapsed);
    DASH_ASSERT_EQ(local_array + local_size, copy_lend,
                   "Unexpected end of copied range");
#ifndef DASH_ENABLE_ASSERTIONS
    dash__unused(copy_lend);
#endif
    delete[] local_array;
  }

  dash::barrier();
  return (static_cast<double>(local_size) / elapsed);
}

double copy_no_local(size_t size, bool parallel)
{
  Array_t global_array(size, dash::BLOCKED);
  size_t  block_size       = global_array.pattern().local_size();
  auto    remote_block     = global_array.pattern().block(dash::size() - 2);
  auto    remote_start_idx = remote_block.offset(0);
  auto    remote_end_idx   = remote_start_idx + block_size;

  size_t  num_copy_elem  = block_size;
  auto    timer_start    = Timer::Now();
  double  elapsed        = 0;

  DASH_LOG_DEBUG("copy_no_local()",
                 "size:",   size,
                 "l_size:", local_size,
                 "n_copy:", num_copy_elem);

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if(dash::myid() == 0 && !parallel) {
    ElementType * local_array = new ElementType[num_copy_elem];
    // Pointer to first element in destination range:
    auto dest_first = local_array;
    // Pointer past last element in destination range:
    auto dest_last  = local_array;
    // Start timer:
    timer_start = Timer::Now();
    // Copy elements in front of local range:
    dest_last = dash::copy(
                  global_array.begin() + remote_start_idx ,
                  global_array.begin() + remote_end_idx,
                  dest_first);
    elapsed = Timer::ElapsedSince(timer_start);

    DASH_LOG_DEBUG("copy_no_local()",
                   "r_start_idx:",     remote_start_idx,
                   "r_end_idx:",       remote_end_idx,
                   "block size:",      block_size,
                   "copied elements:", (dest_last - dest_first),
                   "elapsed us:",      elapsed);
#ifndef DASH_ENABLE_ASSERTIONS
    dash__unused(dest_last);
#endif

    DASH_ASSERT_EQ(
      local_array + num_copy_elem,
      dest_last,
      "Unexpected output pointer from dash::copy");

    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_no_local",
      "Waiting for completion of copy operation");
  dash::barrier();

  return (num_copy_elem / elapsed);
}

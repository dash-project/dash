/**
 * Local copy benchmark for various containers
 *
 * author(s): Felix Moessbauer, LMU Munich */
/* @DASH_HEADER@ */

#define DASH__ALGORITHM__COPY__USE_WAIT

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

double copy_local_to_local(size_t size, int num_repeats);
double copy_shmem_to_local(size_t size, int num_repeats);
double copy_remote_to_local(size_t size, int num_repeats);

void print_measurement_header();
void print_measurement_record(
  const std::string & scenario,
  size_t size,
  int    num_repeats,
  double sec_total,
  double kps);

int main(int argc, char** argv)
{
  double kps;
  double time_s;
  size_t size;
  size_t num_iterations = 10;
  int    num_repeats    = 1000;
  auto   ts_start       = Timer::Now();

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  print_measurement_header();

  /// Increments of 1 GB of elements in total:
  size_t size_inc = static_cast<size_t>(std::pow(2, 30)) /
                    sizeof(ElementType);
  size_t size_min = 7 * size_inc;

  for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
    size     = size_min + ((iteration + 1) * size_inc);

    ts_start = Timer::Now();
    kps      = copy_local_to_local(size, num_repeats);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", size, num_repeats, time_s, kps);

    ts_start = Timer::Now();
    kps      = copy_shmem_to_local(size, num_repeats);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("shmem", size, num_repeats, time_s, kps);

    ts_start = Timer::Now();
    kps      = copy_remote_to_local(size, num_repeats);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote", size, num_repeats, time_s, kps);
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_local_to_local(size_t size, int num_repeats)
{
  Array_t global_array(size, dash::BLOCKED);

  index_t copy_start_idx = global_array.pattern().lbegin();
  index_t copy_end_idx   = global_array.pattern().lend();
  size_t  block_size     = copy_end_idx - copy_start_idx;
  double  elapsed        = 1;

  DASH_LOG_DEBUG("copy_local_to_local()",
                 "size:",             size,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if (dash::myid() == 0) {
    ElementType * local_array = new ElementType[block_size];
    auto timer_start = Timer::Now();
    for (int r = 0; r < num_repeats; ++r) {
      ElementType * copy_lend = dash::copy(global_array.begin() + copy_start_idx,
                                           global_array.begin() + copy_end_idx,
                                           local_array);
      DASH_ASSERT_EQ(local_array + block_size, copy_lend,
                     "Unexpected end of copied range");
#ifndef DASH_ENABLE_ASSERTIONS
      dash__unused(copy_lend);
#endif
    }
    elapsed = Timer::ElapsedSince(timer_start);
    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_local_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();
  return (static_cast<double>(block_size * num_repeats) / elapsed);
}

double copy_shmem_to_local(size_t size, int num_repeats)
{
  Array_t global_array(size, dash::BLOCKED);

  auto    block_size       = global_array.pattern().local_size();
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  auto    remote_block_idx = (dash::myid() + 14) % dash::size();
  auto    remote_block     = global_array.pattern().block(remote_block_idx);
  index_t copy_start_idx   = remote_block.offset(0);
  index_t copy_end_idx     = copy_start_idx + block_size;
  double  elapsed          = 1;

  DASH_LOG_DEBUG("copy_shmem_to_local()",
                 "size:",             size,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if (dash::myid() == 0) {
    ElementType * local_array = new ElementType[block_size];
    auto timer_start = Timer::Now();
    for (int r = 0; r < num_repeats; ++r) {
      ElementType * copy_lend = dash::copy(global_array.begin() + copy_start_idx,
                                           global_array.begin() + copy_end_idx,
                                           local_array);
      DASH_ASSERT_EQ(local_array + block_size, copy_lend,
                     "Unexpected end of copied range");
#ifndef DASH_ENABLE_ASSERTIONS
      dash__unused(copy_lend);
#endif
    }
    elapsed = Timer::ElapsedSince(timer_start);
    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_shmem_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();
  return (static_cast<double>(block_size * num_repeats) / elapsed);
}

double copy_remote_to_local(size_t size, int num_repeats)
{
  Array_t global_array(size, dash::BLOCKED);

  size_t  block_size     = global_array.pattern().local_size();
  // Viewspec of block to copy, using block preceeding last block
  // which is guaranteed to be completely filled;
  auto    remote_block   = global_array.pattern().block(dash::size() - 2);
  index_t copy_start_idx = remote_block.offset(0);
  index_t copy_end_idx   = copy_start_idx + block_size;
  double  elapsed        = 1;

  DASH_LOG_DEBUG("copy_remote_to_local()",
                 "size:",             size,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if(dash::myid() == 0) {
    ElementType * local_array = new ElementType[block_size];
    // Start timer:
    auto timer_start = Timer::Now();
    for (int r = 0; r < num_repeats; ++r) {
      ElementType * copy_lend = dash::copy(
                                  global_array.begin() + copy_start_idx ,
                                  global_array.begin() + copy_end_idx,
                                  local_array);
      DASH_ASSERT_EQ(local_array + block_size, copy_lend,
                     "Unexpected end of copied range");
#ifndef DASH_ENABLE_ASSERTIONS
      dash__unused(copy_lend);
#endif
    }
    elapsed = Timer::ElapsedSince(timer_start);
    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_remote_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();
  return (static_cast<double>(block_size * num_repeats) / elapsed);
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << "bench.07.local-copy"
         << endl
         << endl
         << std::setw(5)  << "units"     << ","
         << std::setw(10) << "scenario"  << ","
         << std::setw(9)  << "repeats"   << ","
         << std::setw(12) << "blocksize" << ","
         << std::setw(9)  << "glob.mb"   << ","
         << std::setw(9)  << "mb/rank"   << ","
         << std::setw(9)  << "time.s"    << ","
         << std::setw(12) << "elem.m/s"
         << endl;
  }
}

void print_measurement_record(
  const std::string & scenario,
  size_t size,
  int    num_repeats,
  double time_s,
  double kps)
{
  double mem_glob = ((static_cast<double>(size) *
                      sizeof(ElementType)) / 1024) / 1024;
  double mem_rank = mem_glob / dash::size();
  if (dash::myid() == 0) {
    cout << std::setw(5)  << dash::size()        << ","
         << std::setw(10) << scenario            << ","
         << std::setw(9)  << num_repeats         << ","
         << std::setw(12) << size / dash::size() << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_glob << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_rank << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << time_s   << ","
         << std::fixed << std::setprecision(2) << std::setw(12) << kps
         << endl;
  }
}

/**
 * Local copy benchmark for various containers
 *
 * author(s): Felix Moessbauer, LMU Munich */
/* @DASH_HEADER@ */

// #define DASH__ALGORITHM__COPY__USE_WAIT

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

#ifndef DASH__ALGORITHM__COPY__USE_WAIT
const std::string dash_copy_variant = "flush";
#else
const std::string dash_copy_variant = "wait";
#endif

double copy_block_to_local(size_t size, int num_repeats, index_t block_index);

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
  size_t num_iterations  = 10;
  int    num_repeats     = 500;
  auto   ts_start        = Timer::Now();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t numa_node_cores = 7;
  // Number of physical cores on a single socket (14 on SuperMUC):
  size_t socket_cores    = 14;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  print_measurement_header();

  /// Increments of 1 GB of elements in total:
  size_t size_inc = static_cast<size_t>(std::pow(2, 30)) /
                    sizeof(ElementType);
  size_t size_min = 7 * size_inc;

  for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
    size     = size_min + ((iteration + 1) * size_inc);

    // Copy first block in array, assigned to unit 0:
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, 0);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", size, num_repeats, time_s, kps);

    // Copy last block in the master's NUMA domain:
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats,
                                   (numa_node_cores-1) % dash::size());
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("uma", size, num_repeats, time_s, kps);

    // Copy first block in the master's neighbor NUMA domain:
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats,
                                   numa_node_cores % dash::size());
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("numa", size, num_repeats, time_s, kps);

    // Copy first block in next socket on the master's node:
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats,
                                   socket_cores % dash::size());
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket", size, num_repeats, time_s, kps);

    // Copy block preceeding last block as it is guaranteed to be located on
    // a remote unit and completely filled:
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, dash::size() - 2);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote", size, num_repeats, time_s, kps);
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_block_to_local(size_t size, int num_repeats, index_t block_index)
{
  Array_t global_array(size, dash::BLOCKED);

  auto    block_size       = global_array.pattern().local_size();
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  auto    source_block     = global_array.pattern().block(block_index);
  index_t copy_start_idx   = source_block.offset(0);
  index_t copy_end_idx     = copy_start_idx + block_size;
  double  elapsed          = 1;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:"       block_index,
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
      "copy_block_to_local",
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
         << std::setw(10) << "copy type" << ","
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
         << std::setw(10) << dash_copy_variant   << ","
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

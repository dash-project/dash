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
#include <string>
#include <math.h>
#include <unistd.h>

using std::cout;
using std::endl;
using std::vector;
using std::string;

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

typedef struct {
  int  rank;
  char host[100];
  int  cpu;
  int  numa_node;
} unit_pin_info;

std::ostream & operator<<(
  std::ostream        & os,
  const unit_pin_info & upi)
{
  std::ostringstream ss;
  ss << "unit_pin_info("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

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
  int    unit_src,
  size_t size,
  int    num_repeats,
  double sec_total,
  double kps);

int main(int argc, char** argv)
{
  double kps;
  double time_s;
  size_t size;
  size_t num_iterations  = 5;
  int    num_repeats     = 200;
  auto   ts_start        = Timer::Now();
  size_t num_numa_nodes  = dash::util::Locality::NumNumaNodes();
  size_t num_local_cpus  = dash::util::Locality::NumCPUs();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t numa_node_cores = num_local_cpus / num_numa_nodes;
  // Number of physical cores on a single socket (14 on SuperMUC):
  size_t socket_cores    = numa_node_cores * 2;

  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  // Collect process pinning information:
  //
  dash::Array<unit_pin_info> unit_pinning(dash::size());

  int cpu        = dash::util::Locality::UnitCPU();
  int numa_node  = dash::util::Locality::UnitNUMANode();

  unit_pin_info my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  gethostname(my_pin_info.host, 100);

  unit_pinning[dash::myid()] = my_pin_info;

  dash::barrier();

  if (dash::myid() == 0) {
    cout << std::setw(5)  << "unit"      << " "
         << std::setw(20) << "host"      << " "
         << std::setw(10) << "numa node" << " "
         << std::setw(5)  << "cpu"
         << endl;
    for (dart_unit_t unit = 0; unit < dash::size(); ++unit) {
      unit_pin_info pin_info = unit_pinning[unit];
      cout << std::setw(5)  << pin_info.rank      << " "
           << std::setw(20) << pin_info.host      << " "
           << std::setw(10) << pin_info.numa_node << " "
           << std::setw(5)  << pin_info.cpu
           << endl;
    }
    cout << "number of NUMA nodes: " << num_numa_nodes
         << endl
         << "local CPUs:           " << num_local_cpus
         << endl
         << "cores per NUMA node:  " << numa_node_cores
         << endl
         << "cores per socket:     " << socket_cores
         << endl;
  }

  dash::barrier();

  print_measurement_header();

  /// Increments of 1 GB of elements in total:
  size_t size_inc = 10 * (static_cast<size_t>(std::pow(2, 30)) /
                          sizeof(ElementType));
  size_t size_min = 1 * size_inc;

  dart_unit_t unit_src;
  for (size_t iteration = 0; iteration < num_iterations; ++iteration) {
    size     = size_min + (iteration * size_inc);

    // Copy first block in array, assigned to unit 0:
    unit_src = 0;
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", unit_src, size, num_repeats,
                             time_s, kps);

    // Copy last block in the master's NUMA domain:
    unit_src = (numa_node_cores-1) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("uma", unit_src, size, num_repeats,
                             time_s, kps);

    // Copy block in the master's neighbor NUMA domain:
    unit_src = (numa_node_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("numa", unit_src, size, num_repeats,
                             time_s, kps);

    // Copy first block in next socket on the master's node:
    unit_src = (socket_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket", unit_src, size, num_repeats,
                             time_s, kps);

    // Copy block preceeding last block as it is guaranteed to be located on
    // a remote unit and completely filled:
    unit_src = dash::size() - 2;
    ts_start = Timer::Now();
    kps      = copy_block_to_local(size, num_repeats, unit_src);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote", unit_src, size, num_repeats,
                             time_s, kps);
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
  auto    source_unit_id   = global_array.pattern().unit_at(copy_start_idx);
  double  elapsed          = 1;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:",      block_index,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  if (source_unit_id != block_index) {
    DASH_THROW(dash::exception::RuntimeError,
               "copy_block_to_local: Invalid distribution of global array");
    return 0;
  }

  for (size_t l = 0; l < global_array.lsize(); ++l) {
    global_array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  dash::barrier();

  if(dash::myid() == 0) {
    ElementType * local_array = new ElementType[block_size];

    // Perform measurement:
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

    // Validate values:
    for (size_t l = 0; l < block_size; ++l) {
      if (local_array[l] != ((source_unit_id + 1) * 1000) + l) {
        DASH_THROW(dash::exception::RuntimeError,
                   "copy_block_to_local: Validation failed");
        return 0;
      }
    }

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
         << std::setw(10) << "mpi impl"  << ","
         << std::setw(10) << "copy type" << ","
         << std::setw(10) << "scenario"  << ","
         << std::setw(9)  << "src.unit"  << ","
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
  int    unit_src,
  size_t size,
  int    num_repeats,
  double time_s,
  double kps)
{
  if (dash::myid() == 0) {
    string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    double mem_glob = ((static_cast<double>(size) *
                        sizeof(ElementType)) / 1024) / 1024;
    double mem_rank = mem_glob / dash::size();
    cout << std::setw(5)  << dash::size()        << ","
         << std::setw(10) << mpi_impl            << ","
         << std::setw(10) << dash_copy_variant   << ","
         << std::setw(10) << scenario            << ","
         << std::setw(9)  << unit_src            << ","
         << std::setw(9)  << num_repeats         << ","
         << std::setw(12) << size / dash::size() << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_glob << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << mem_rank << ","
         << std::fixed << std::setprecision(2) << std::setw(9)  << time_s   << ","
         << std::fixed << std::setprecision(2) << std::setw(12) << kps
         << endl;
  }
}

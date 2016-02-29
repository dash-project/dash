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
#include <cstring>

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#endif

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

typedef double  ElementType;
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
const std::string dash_async_copy_variant = "flush";
#else
const std::string dash_async_copy_variant = "wait";
#endif

typedef typename dash::util::BenchmarkParams::config_params_type
  bench_cfg_params;

typedef struct benchmark_params_t {
  size_t size_base;
  size_t size_min;
  size_t num_iterations;
  size_t num_repeats;
  size_t rep_base;
  bool   verify;
  bool   local_only;
} benchmark_params;

typedef enum local_copy_method_t {
  MEMCPY,
  STD_COPY,
  DASH_COPY,
  DASH_COPY_ASYNC
} local_copy_method;

double copy_block_to_local(
  size_t                   size,
  int                      repeat,
  int                      num_repeats,
  index_t                  source_unit_id,
  index_t                  target_unit_id,
  const benchmark_params & params,
  local_copy_method        l_copy_method = DASH_COPY);

void print_measurement_header();
void print_measurement_record(
  const std::string      & scenario,
  const std::string      & local_copy_method,
  const bench_cfg_params & cfg_params,
  int                      unit_src,
  int                      unit_dest,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  double                   mbps,
  const benchmark_params & params);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);
#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "off");
  MPI_Pcontrol(0, "clear");
#endif

  Timer::Calibrate(0);

  double mbps;
  double time_s;
  auto   ts_start        = Timer::Now();
  size_t num_numa_nodes  = dash::util::Locality::NumNumaNodes();
  size_t num_local_cpus  = dash::util::Locality::NumCPUs();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t numa_node_cores = num_local_cpus / num_numa_nodes;
  // Number of physical cores on a single socket (14 on SuperMUC):
  size_t socket_cores    = numa_node_cores * 2;
  // Number of processing nodes:
  size_t num_nodes       = dash::util::Locality::NumNodes();

  dash::util::BenchmarkParams bench_params("bench.07.local-copy");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  size_t num_iterations   = params.num_iterations;
  size_t num_repeats      = params.num_repeats;
  size_t size_inc         = params.size_base;
  size_t size_min         = params.size_min;

  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);

  print_measurement_header();

  dart_unit_t u_src;
  dart_unit_t u_dst;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = size_min + (std::pow(params.rep_base,i) * size_inc);
    auto size       = block_size * dash::size();

    // Copy first block in array, assigned to unit 0, using memcpy:
    u_src    = 0;
    u_dst    = 0;
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params,
                                   MEMCPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "memcpy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    // Copy first block in array, assigned to unit 0, using std::copy:
    u_src    = 0;
    u_dst    = 0;
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params,
                                   STD_COPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "stdcopy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    // Copy first block in array, assigned to unit 0:
    u_src    = 0;
    u_dst    = 0;
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    // Copy last block in the master's NUMA domain:
    u_src    = 0;
    u_dst    = (numa_node_cores-1) % dash::size();
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("uma", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    // Copy block in the master's neighbor NUMA domain:
    u_src    = 0;
    u_dst    = (numa_node_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("numa", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    // Copy first block in next socket on the master's node:
    u_src    = 0;
    u_dst    = (socket_cores + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_repeats, time_s, mbps, params);

    if (params.local_only || num_nodes < 2) {
      continue;
    }

    // Limit number of repeats for remote copying:
    auto num_r_repeats = std::min<size_t>(num_repeats, 10000);

    // Copy block preceeding last block as it is guaranteed to be located on
    // a remote unit and completely filled:
    u_src    = 0;
    u_dst    = dash::size() - 2;
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.1", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_r_repeats, time_s, mbps, params);
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, params,
                                   DASH_COPY_ASYNC);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.1", "dash::acopy", bench_cfg, u_src, u_dst,
                             size, num_r_repeats, time_s, mbps, params);
    if (num_nodes < 3) {
      continue;
    }
    u_src    = 0;
    u_dst    = dash::size() / 2;
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.2", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_r_repeats, time_s, mbps, params);
    if (num_nodes < 4) {
      continue;
    }
    u_src    = 0;
    u_dst    = ((num_local_cpus * 2) + (numa_node_cores / 2)) % dash::size();
    ts_start = Timer::Now();
    mbps     = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.3", "dash::copy", bench_cfg, u_src, u_dst,
                             size, num_r_repeats, time_s, mbps, params);
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

double copy_block_to_local(
  size_t                   size,
  int                      repeat,
  int                      num_repeats,
  index_t                  source_unit_id,
  index_t                  target_unit_id,
  const benchmark_params & params,
  local_copy_method        l_copy_method)
{
  Array_t global_array(size, dash::BLOCKED);
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  index_t block_index     = source_unit_id;
  auto    source_block    = global_array.pattern().block(block_index);
  size_t  block_size      = source_block.size();
  index_t copy_start_idx  = source_block.offset(0);
  index_t copy_end_idx    = copy_start_idx + block_size;
  auto    block_unit_id   = global_array.pattern().unit_at(copy_start_idx);

  dash::Shared<double> elapsed;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:",      block_index,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  if (source_unit_id != block_unit_id) {
    DASH_THROW(dash::exception::RuntimeError,
               "copy_block_to_local: Invalid distribution of global array");
    return 0;
  }

  double elapsed_us = 0;
  // Perform measurement:
  ElementType * local_array;
  if(dash::myid() == target_unit_id) {
    ElementType * local_array = new ElementType[block_size];
  }
  for (int r = 0; r < num_repeats; ++r) {
    ElementType * copy_lend = nullptr;
    ElementType * src_begin = nullptr;
    Timer::timestamp_t ts_start = 0;

    std::srand(dash::myid() * 42 + repeat);
    for (size_t l = 0; l < global_array.lsize(); ++l) {
      global_array.local[l] = ((dash::myid() + 1) * 100000)
                              + (l * 1000)
                              + (std::rand() * 1.0e-9);
    }
    dash::barrier();
#ifdef DASH_ENABLE_IPM
    MPI_Pcontrol(0, "on");
#endif
    if(dash::myid() == target_unit_id) {
      ElementType * local_array = new ElementType[block_size];

      switch (l_copy_method) {
        case STD_COPY:
          src_begin   = (global_array.begin() + copy_start_idx).local();
          ts_start    = Timer::Now();
          copy_lend   = std::copy(src_begin,
                                  src_begin + block_size,
                                  local_array);
          elapsed_us += Timer::ElapsedSince(ts_start);
          break;
        case MEMCPY:
          src_begin   = (global_array.begin() + copy_start_idx).local();
          copy_lend   = local_array + block_size;
          ts_start    = Timer::Now();
          memcpy(local_array, src_begin, block_size * sizeof(ElementType));
          elapsed_us += Timer::ElapsedSince(ts_start);
          break;
        case DASH_COPY_ASYNC:
          ts_start    = Timer::Now();
          copy_lend = dash::copy_async(global_array.begin() + copy_start_idx,
                                       global_array.begin() + copy_end_idx,
                                       local_array).get();
          elapsed_us += Timer::ElapsedSince(ts_start);
          break;
        default:
          ts_start    = Timer::Now();
          copy_lend   = dash::copy(global_array.begin() + copy_start_idx,
                                   global_array.begin() + copy_end_idx,
                                   local_array);
          elapsed_us += Timer::ElapsedSince(ts_start);
          break;
      }
#ifdef DASH_ENABLE_IPM
      MPI_Pcontrol(0, "off");
#endif
      // Validate values:
      if (copy_lend != local_array + block_size) {
        DASH_THROW(dash::exception::RuntimeError,
                   "copy_block_to_local: " <<
                   "Unexpected end of copy output range " <<
                   "expected: " << local_array + block_size << " " <<
                   "actual: "   << copy_lend);
      }
      if (params.verify) {
        for (size_t l = 0; l < block_size; ++l) {
          ElementType expected = global_array[copy_start_idx + l];
          ElementType actual   = local_array[l];
          if (actual != expected) {
            DASH_THROW(dash::exception::RuntimeError,
                       "copy_block_to_local: Validation failed " <<
                       "for copied element at offset " << l << ": " <<
                       "expected: " << expected << " " <<
                       "actual: "   << actual);
            return 0;
          }
        }
      }
    }
  }
  if(dash::myid() == target_unit_id) {
    elapsed.set(elapsed_us);
    delete[] local_array;
  }

  DASH_LOG_DEBUG(
      "copy_block_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();

  double mkeys_per_sec = (static_cast<double>(block_size * num_repeats)
                         / elapsed.get());
  return mkeys_per_sec * sizeof(ElementType);
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw(5)  << "units"      << ","
         << std::setw(10) << "mpi.impl"   << ","
         << std::setw(10) << "scenario"   << ","
         << std::setw(12) << "copy.type"  << ","
         << std::setw(12) << "acopy.type" << ","
         << std::setw(11) << "src.unit"   << ","
         << std::setw(11) << "dest.unit"  << ","
         << std::setw(9)  << "repeats"    << ","
         << std::setw(12) << "blocksize"  << ","
         << std::setw(9)  << "glob.mb"    << ","
         << std::setw(9)  << "mb/block"   << ","
         << std::setw(9)  << "time.s"     << ","
         << std::setw(12) << "mb/s"
         << endl;
  }
}

void print_measurement_record(
  const std::string      & scenario,
  const std::string      & local_copy_method,
  const bench_cfg_params & cfg_params,
  int                      unit_src,
  int                      unit_dest,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  double                   mbps,
  const benchmark_params & params)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    double mem_g = ((static_cast<double>(size) *
                     sizeof(ElementType)) / 1024) / 1024;
    double mem_l = mem_g / dash::size();
    cout << std::right
         << std::setw(5)  << dash::size()              << ","
         << std::setw(10) << mpi_impl                  << ","
         << std::setw(10) << scenario                  << ","
         << std::setw(12) << local_copy_method         << ","
         << std::setw(12) << dash_async_copy_variant   << ","
         << std::setw(11) << unit_src                  << ","
         << std::setw(11) << unit_dest                 << ","
         << std::setw(9)  << num_repeats               << ","
         << std::setw(12) << size / dash::size()       << ","
         << std::fixed << setprecision(2) << setw(9)  << mem_g << ","
         << std::fixed << setprecision(2) << setw(9)  << mem_l << ","
         << std::fixed << setprecision(2) << setw(9)  << secs  << ","
         << std::fixed << setprecision(2) << setw(12) << mbps
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  // Minimum block size of 4 KB:
  params.size_base      = (4 * 1024) / sizeof(ElementType);
  params.num_iterations = 8;
  params.rep_base       = 4;
  params.num_repeats    = 0;
  params.verify         = false;
  params.local_only     = false;
  params.size_min       = 0;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      params.size_base      = atoi(argv[i+1]);
    } else if (flag == "-smin") {
      params.size_min       = atoi(argv[i+1]);
    } else if (flag == "-i") {
      params.num_iterations = atoi(argv[i+1]);
    } else if (flag == "-r") {
      params.num_repeats    = atoi(argv[i+1]);
    } else if (flag == "-rb") {
      params.rep_base       = atoi(argv[i+1]);
    } else if (flag == "-verify") {
      params.verify         = true;
      --i;
    } else if (flag == "-lo") {
      params.local_only     = true;
      --i;
    }
  }
  if (params.num_repeats == 0) {
    params.num_repeats = 2 * std::pow(params.rep_base, 10);
  }
  return params;
}

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params)
{
  if (dash::myid() != 0) {
    return;
  }

  bench_cfg.print_section_start("Runtime arguments");
  bench_cfg.print_param("-sb",     "block size base", params.size_base);
  bench_cfg.print_param("-smin",   "min. block size", params.size_min);
  bench_cfg.print_param("-i",      "iterations",      params.num_iterations);
  bench_cfg.print_param("-r",      "repeats",         params.num_repeats);
  bench_cfg.print_param("-rb",     "rep. base",       params.rep_base);
  bench_cfg.print_param("-verify", "verification",    params.verify);
  bench_cfg.print_param("-lo",     "local only",      params.local_only);
  bench_cfg.print_section_end();
}


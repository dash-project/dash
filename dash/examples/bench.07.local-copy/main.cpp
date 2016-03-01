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

#include <malloc.h>

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
  size_t min_repeats;
  size_t rep_base;
  bool   verify;
  bool   local_only;
  bool   flush_l3_cache;
} benchmark_params;

typedef enum local_copy_method_t {
  MEMCPY,
  STD_COPY,
  DASH_COPY,
  DASH_COPY_ASYNC
} local_copy_method;

typedef struct measurement_t {
  double time_copy_s;
  double time_init_s;
  double mb_per_s;
} measurement;

measurement copy_block_to_local(
  size_t                   size,
  int                      repeat,
  int                      num_repeats,
  index_t                  source_unit_id,
  index_t                  target_unit_id,
  index_t                  init_unit_id,
  const benchmark_params & params,
  local_copy_method        l_copy_method = DASH_COPY);

void print_measurement_header();
void print_measurement_record(
  const std::string      & scenario,
  const std::string      & local_copy_method,
  const bench_cfg_params & cfg_params,
  int                      unit_src,
  int                      unit_dest,
  int                      unit_init,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  measurement              measurement,
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

  measurement res;
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
  size_t size_inc         = params.size_min;

  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);

  print_measurement_header();

  // Unit that owns the elements to be copied:
  dart_unit_t u_src;
  // Unit that creates the local copy:
  dart_unit_t u_dst;
  // Unit that initializes the array range to be copied at the source unit:
  dart_unit_t u_init;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = std::pow(params.size_base,i) * size_inc;
    auto size       = block_size * dash::size();

    // Copy first block in array, assigned to unit 0, using memcpy:
    u_src    = 0;
    u_dst    = 0;
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params, MEMCPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "memcpy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    // Copy first block in array, assigned to unit 0, using std::copy:
    u_src    = 0;
    u_dst    = 0;
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params, STD_COPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "stdcopy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    // Copy first block in array, assigned to unit 0:
    u_src    = 0;
    u_dst    = 0;
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    // Copy last block in the master's NUMA domain:
    u_src    = 0;
    u_dst    = (numa_node_cores-1) % dash::size();
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("uma", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    // Copy block in the master's neighbor NUMA domain:
    u_src    = 0;
    u_dst    = (numa_node_cores + (numa_node_cores / 2)) % dash::size();
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("numa", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    // Copy first block in next socket on the master's node:
    u_src    = 0;
    u_dst    = (socket_cores + (numa_node_cores / 2)) % dash::size();
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);

    if (params.local_only || num_nodes < 2) {
      continue;
    }

    // Limit number of repeats for remote copying:
    auto num_r_repeats = std::min<size_t>(num_repeats, params.min_repeats);

    // Copy block preceeding last block as it is guaranteed to be located on
    // a remote unit and completely filled:
    u_src    = 0;
    u_dst    = dash::size() - 2;
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.1", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
    if (num_nodes < 3) {
      continue;
    }
    u_src    = 0;
    u_dst    = dash::size() / 2;
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.2", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
    if (num_nodes < 4) {
      continue;
    }
    u_src    = 0;
    u_dst    = ((num_local_cpus * 2) + (numa_node_cores / 2)) % dash::size();
    u_init   = (u_dst + num_local_cpus) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_r_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("remote.3", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_r_repeats,
                             time_s, res, params);
  }

  DASH_PRINT_MASTER("Benchmark finished");

  dash::finalize();
  return 0;
}

measurement copy_block_to_local(
  size_t                   size,
  int                      repeat,
  int                      num_repeats,
  index_t                  source_unit_id,
  index_t                  target_unit_id,
  index_t                  init_unit_id,
  const benchmark_params & params,
  local_copy_method        l_copy_method)
{
  measurement result;
  result.time_init_s = 0;
  result.time_copy_s = 0;
  result.mb_per_s    = 0;

  Array_t global_array(size, dash::BLOCKED);
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  index_t block_index       = source_unit_id;
  auto    source_block      = global_array.pattern().block(block_index);
  size_t  block_size        = source_block.size();
  size_t  block_bytes       = block_size * sizeof(ElementType);
  index_t copy_start_idx    = source_block.offset(0);
  index_t copy_end_idx      = copy_start_idx + block_size;
  auto    block_unit_id     = global_array.pattern().unit_at(copy_start_idx);
  size_t  num_numa_nodes    = dash::util::Locality::NumNumaNodes();
  size_t  num_local_cpus    = dash::util::Locality::NumCPUs();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t  numa_node_cores   = num_local_cpus / num_numa_nodes;
  // Global pointer to copy input begin:
  auto    src_g_begin       = global_array.begin() + copy_start_idx;
  // Global pointer to copy input end:
  auto    src_g_end         = global_array.begin() + copy_end_idx;
  // Size of shared cache on highest locality level, in bytes:
  size_t  l3_cache_size     = dash::util::Locality::CacheSizes().back();
  // Alignment:
  size_t  align_size        = 128;
  // Buffer size required to allocate by every unit to occupy the entire L3d
  // cache, i.e. balance size of L3 cache over units, assuming L3 cache is shared
  // on NUMA node level:
  size_t  l3_local_size     = l3_cache_size / numa_node_cores;
  // Ensure l3_local_size is multiple of alignment:
  l3_local_size = (l3_local_size % align_size == 0)
                  ? l3_local_size
                  : dash::math::div_ceil(l3_local_size, align_size) * align_size;
  // Local pointer to copy input begin, or nullptr if not local:
  ElementType * src_l_begin = src_g_begin.local();

  // Total time spent in copy operations:
  dash::Shared<double> time_copy_us;
  // Total time spent in initialization of array values:
  dash::Shared<double> time_init_us;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:",      block_index,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  if (source_unit_id != block_unit_id) {
    DASH_THROW(dash::exception::RuntimeError,
               "copy_block_to_local: Invalid distribution of global array");
    return result;
  }

  // Prepare buffers:
  ElementType * local_array = nullptr;
  if (dash::myid() == target_unit_id) {
    local_array     = static_cast<ElementType *>(
                        memalign(align_size, block_bytes));
  }
  ElementType * cache_overwrite = nullptr;
  if (params.flush_l3_cache) {
    cache_overwrite = static_cast<ElementType *>(
                        memalign(align_size, l3_local_size));
  }

  double copy_us = 0;
  double init_us = 0;
  // Perform measurement:
  for (int r = 0; r < num_repeats; ++r) {
    dash::barrier();
    Timer::timestamp_t ts_init_start = Timer::Now();
    // ------------------------------------------------------------------------
    // -- Initialize global array: --------------------------------------------
    for (size_t l = 0; l < block_size; ++l) {
      global_array.local[l] = 1000 + dash::myid();
    }
    dash::barrier();
    // -- Prevent copying from cache: -----------------------------------------
    if (params.flush_l3_cache) {
      // -- Prevent copying from cache by overwriting entire L3d: -------------
      for (size_t i = 0; i < l3_local_size / sizeof(ElementType); ++i) {
        cache_overwrite[i] = std::rand() * 1.0e-9;
      }
    } else {
      // -- Prevent copying from L3 cache using remote initialization: --------
      //    Initialize values in copy input range:
      //    Initialize values at unit on different node than target unit:
      if(dash::myid() == init_unit_id) {
        std::srand(dash::myid() * 42 + repeat);
        // Initialize block values in local array:
        ElementType * block_values = new ElementType[block_size];
        for (size_t p = 0; p < block_size; ++p) {
          block_values[p] = ((target_unit_id + 1) * 100000)
                            + (p * 1000)
                            + (std::rand() * 1.0e-9);
        }
        // Copy block values to global array:
        dash::copy(block_values,
                   block_values + block_size,
                   src_g_begin);
        // Free local block values after they have been copied to source block:
        delete[] block_values;
      }
    }
    dash::barrier();
    // -- Finished initialization of global array. ----------------------------
    // ------------------------------------------------------------------------

#ifdef DASH_ENABLE_IPM
    MPI_Pcontrol(0, "on");
#endif
    // -- Copy array block from source to destination rank: -------------------
    if(dash::myid() == target_unit_id) {
      init_us += Timer::ElapsedSince(ts_init_start);

      ElementType * copy_lend     = nullptr;
      Timer::timestamp_t ts_start = Timer::Now();
      switch (l_copy_method) {
        case STD_COPY:
          copy_lend = std::copy(src_l_begin,
                                src_l_begin + block_size,
                                local_array);
          break;
        case MEMCPY:
          copy_lend = local_array + block_size;
          memcpy(local_array, src_l_begin, block_size * sizeof(ElementType));
          break;
        case DASH_COPY_ASYNC:
          copy_lend = dash::copy_async(src_g_begin,
                                       src_g_end,
                                       local_array).get();
          break;
        default:
          copy_lend = dash::copy(src_g_begin,
                                 src_g_end,
                                 local_array);
          break;
      }
      // -- Finished copy from source to destination rank. --------------------
      copy_us += Timer::ElapsedSince(ts_start);
      // -- Validate values: --------------------------------------------------
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
                       "for copied element at offset " << l << "  " <<
                       "in repetition " << r << ": " <<
                       "expected: "     << expected << " " <<
                       "actual: "       << actual);
            return result;
          }
        }
      }
      // -- Finished validation. ----------------------------------------------
    } // if target unit
#ifdef DASH_ENABLE_IPM
    MPI_Pcontrol(0, "off");
#endif
    // Wait for validation, otherwise values in global array could be
    // overwritten when other units start with next repetition:
    dash::barrier();
  } // for repeats

  // Free buffers:
  if (cache_overwrite != nullptr) {
    free(cache_overwrite);
  }
  if (local_array != nullptr) {
    free(local_array);
  }

  if(dash::myid() == target_unit_id) {
    time_copy_us.set(copy_us);
    time_init_us.set(init_us);
  }

  DASH_LOG_DEBUG(
      "copy_block_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();

  double mkeys_per_sec = (static_cast<double>(block_size * num_repeats)
                         / time_copy_us.get());

  result.time_copy_s   = time_copy_us.get() * 1.0e-6;
  result.time_init_s   = time_init_us.get() * 1.0e-6;
  result.mb_per_s      = mkeys_per_sec * sizeof(ElementType);

  return result;
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
         << std::setw(10) << "src.unit"   << ","
         << std::setw(10) << "init.unit"  << ","
         << std::setw(10) << "dest.unit"  << ","
         << std::setw(9)  << "repeats"    << ","
         << std::setw(9)  << "block.n"    << ","
         << std::setw(9)  << "block.kb"   << ","
         << std::setw(9)  << "glob.kb"    << ","
         << std::setw(7)  << "init.s"     << ","
         << std::setw(7)  << "copy.s"     << ","
         << std::setw(7)  << "time.s"     << ","
         << std::setw(9)  << "mb/s"
         << endl;
  }
}

void print_measurement_record(
  const std::string      & scenario,
  const std::string      & local_copy_method,
  const bench_cfg_params & cfg_params,
  int                      unit_src,
  int                      unit_dest,
  int                      unit_init,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  measurement              measurement,
  const benchmark_params & params)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    size_t block_n     = size / dash::size();
    size_t g_size_kb   = (size * sizeof(ElementType)) / 1024;
    size_t block_kb    = (block_n * sizeof(ElementType)) / 1024;
    double mbps        = measurement.mb_per_s;
    double init_s      = measurement.time_init_s;
    double copy_s      = measurement.time_copy_s;
    cout << std::right
         << std::setw(5)  << dash::size()            << ","
         << std::setw(10) << mpi_impl                << ","
         << std::setw(10) << scenario                << ","
         << std::setw(12) << local_copy_method       << ","
         << std::setw(12) << dash_async_copy_variant << ","
         << std::setw(10) << unit_src                << ","
         << std::setw(10) << unit_dest               << ","
         << std::setw(10) << unit_init               << ","
         << std::setw(9)  << num_repeats             << ","
         << std::setw(9)  << block_n                 << ","
         << std::setw(9)  << block_kb                << ","
         << std::setw(9)  << g_size_kb               << ","
         << std::fixed << setprecision(2) << setw(7) << init_s << ","
         << std::fixed << setprecision(2) << setw(7) << copy_s << ","
         << std::fixed << setprecision(2) << setw(7) << secs   << ","
         << std::fixed << setprecision(2) << setw(9) << mbps
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  // Minimum block size of 4 KB:
  params.size_base      = 4;
  params.num_iterations = 8;
  params.rep_base       = params.size_base;
  params.num_repeats    = 0;
  params.min_repeats    = 1;
  params.verify         = true;
  params.local_only     = false;
  params.flush_l3_cache = false;
  params.size_min       = 0;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      params.size_base      = atoi(argv[i+1]);
    } else if (flag == "-smin") {
      params.size_min       = atoi(argv[i+1]);
    } else if (flag == "-i") {
      params.num_iterations = atoi(argv[i+1]);
    } else if (flag == "-rmax") {
      params.num_repeats    = atoi(argv[i+1]);
    } else if (flag == "-rmin") {
      params.min_repeats    = atoi(argv[i+1]);
    } else if (flag == "-rb") {
      params.rep_base       = atoi(argv[i+1]);
    } else if (flag == "-verify") {
      params.verify         = true;
      --i;
    } else if (flag == "-lo") {
      params.local_only     = true;
      --i;
    } else if (flag == "-fcache") {
      params.flush_l3_cache = true;
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
  bench_cfg.print_param("-smin",   "initial block size",  params.size_min);
  bench_cfg.print_param("-sb",     "block size base",     params.size_base);
  bench_cfg.print_param("-rmax",   "initial repeats",     params.num_repeats);
  bench_cfg.print_param("-rmin",   "min, repeats",        params.min_repeats);
  bench_cfg.print_param("-rb",     "rep. base",           params.rep_base);
  bench_cfg.print_param("-i",      "iterations",          params.num_iterations);
  bench_cfg.print_param("-verify", "verification",        params.verify);
  bench_cfg.print_param("-lo",     "local only",          params.local_only);
  bench_cfg.print_param("-fcache", "flush full L3 cache", params.flush_l3_cache);
  bench_cfg.print_section_end();
}


/**
 * Local copy benchmark for various containers
 */

#include <libdash.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <limits>

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#endif

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

static void *_aligned_malloc(size_t size, size_t alignment) {
    void *buffer;
    posix_memalign(&buffer, alignment, size);
    return buffer;
}

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
  bool   flush_cache;
} benchmark_params;

typedef enum local_copy_method_t {
  MEMCPY,
  STD_COPY,
  DASH_COPY,
  DASH_COPY_ASYNC
} local_copy_method;

typedef struct measurement_t {
  double time_copy_s;
  double time_copy_min_us;
  double time_copy_max_us;
  double time_copy_med_us;
  double time_copy_sdv_us;
  double time_init_s;
  double mb_per_s;
} measurement;

measurement copy_block_to_local(
  size_t                   size,
  size_t                   repeat,
  size_t                   num_repeats,
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

  // 0: real, 1: virt
  Timer::Calibrate(0);

  dash::util::UnitLocality uloc;

  measurement res;
  double time_s;
  auto   ts_start        = Timer::Now();
  size_t num_numa_nodes  = uloc.num_numa();
  size_t num_local_cores = uloc.node_domain().num_cores();
  // Number of physical cores in a single NUMA domain (7 on SuperMUC):
  size_t numa_node_cores = num_local_cores / num_numa_nodes;
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
  // Unit used as default destination:
  dart_unit_t u_loc = (numa_node_cores % dash::size());

#if 1
  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = std::pow(params.size_base,i) * size_inc;
    auto size       = block_size * dash::size();

    num_repeats     = std::max<size_t>(num_repeats, params.min_repeats);

    u_src    = u_loc;
    u_dst    = u_loc;
    u_init   = (u_dst + num_local_cores) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params, STD_COPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "std::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
  }
#endif

#if 1
  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = std::pow(params.size_base,i) * size_inc;
    auto size       = block_size * dash::size();

    num_repeats     = std::max<size_t>(num_repeats, params.min_repeats);

    u_src    = u_loc;
    u_dst    = u_loc;
    u_init   = (u_dst + num_local_cores) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("local", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
  }
#endif

#if 1
  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = std::pow(params.size_base,i) * size_inc;
    auto size       = block_size * dash::size();

    num_repeats     = std::max<size_t>(num_repeats, params.min_repeats);

    u_src    = u_loc;
    u_dst    = (u_src + numa_node_cores) % dash::size();
    u_init   = (u_dst + num_local_cores) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params, DASH_COPY);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("socket.b", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
  }
#endif

#if 1
  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    auto block_size = std::pow(params.size_base,i) * size_inc;
    auto size       = block_size * dash::size();

    num_repeats     = std::max<size_t>(num_repeats, params.min_repeats);

    u_src    = u_loc;
    u_dst    = (u_src + num_local_cores) % dash::size();
    u_init   = (u_src + numa_node_cores) % dash::size();
    ts_start = Timer::Now();
    res      = copy_block_to_local(size, i, num_repeats, u_src, u_dst, u_init,
                                   params, DASH_COPY_ASYNC);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record("rmt.async", "dash::copy", bench_cfg,
                             u_src, u_dst, u_init, size, num_repeats,
                             time_s, res, params);
  }
#endif

  if( dash::myid()==0 ) {
    cout << "Benchmark finished" << endl;
  }

  dash::finalize();
  return 0;
}

measurement copy_block_to_local(
  size_t                   size,
  size_t                   iteration,
  size_t                   num_repeats,
  index_t                  source_unit_id,
  index_t                  target_unit_id,
  index_t                  init_unit_id,
  const benchmark_params & params,
  local_copy_method        l_copy_method)
{
  typedef typename Array_t::pattern_type pattern_t;
  pattern_t pattern(size, dash::BLOCKED);

  measurement result;
  result.time_init_s        = 0;
  result.time_copy_s        = 0;
  result.time_copy_min_us   = 0;
  result.time_copy_max_us   = 0;
  result.mb_per_s           = 0;

  dash::util::UnitLocality uloc;

  auto    myid              = dash::myid();
  // Index of block to copy. Use block of succeeding neighbor
  // which is expected to be in same NUMA domain for unit 0:
  index_t block_index       = source_unit_id;
  auto    source_block      = pattern.block(block_index);
  size_t  block_size        = source_block.size();
  size_t  block_bytes       = block_size * sizeof(ElementType);
  index_t copy_start_idx    = source_block.offset(0);
  index_t copy_end_idx      = copy_start_idx + block_size;
  auto    block_unit_id     = pattern.unit_at(copy_start_idx);
  // Size of shared cache on highest locality level, in bytes:
  size_t  cache_size        = uloc.cache_line_size(2);
  // Alignment:
  size_t  align_size        = 128;
  // Ensure cache_size is multiple of alignment:
  cache_size = (cache_size % align_size == 0)
                  ? cache_size
                  : ((cache_size / align_size) + 1) * align_size;

  // Total time spent in copy operations:
  dash::Shared<double> time_copy_us;
  // Total time spent in initialization of array values:
  dash::Shared<double> time_init_us;
  // Minimum duration for a single copy operation:
  dash::Shared<double> time_copy_min_us;
  // Maximum duration for a single copy operation:
  dash::Shared<double> time_copy_max_us;
  // Median of duration of copy operations:
  dash::Shared<double> time_copy_med_us;
  // Standard deviation of duration of copy operations:
  dash::Shared<double> time_copy_sdv_us;

  DASH_LOG_DEBUG("copy_block_to_local()",
                 "size:",             size,
                 "block index:",      block_index,
                 "block size:",       block_size,
                 "copy index range:", copy_start_idx, "-", copy_end_idx);

  if (source_unit_id != block_unit_id) {
    DASH_THROW(dash::exception::RuntimeError,
               "copy_block_to_local: Invalid distribution of global array");
  }

  // Prepare local buffer:
  ElementType * local_array = nullptr;
  if (myid == target_unit_id) {
    local_array = static_cast<ElementType *>(
                    _aligned_malloc(block_bytes, align_size));
  }

  Array_t global_array;
  global_array.allocate(size, dash::BLOCKED);

  std::srand(time(NULL));

  double total_copy_us = 0;
  double total_init_us = 0;
  std::vector<double> history_copy_us;
  // Perform measurement:
  for (size_t r = 0; r < num_repeats; ++r) {
    dash::barrier();
    Timer::timestamp_t ts_init_start = Timer::Now();

    // Global pointer to copy input begin:
    auto    src_g_begin = global_array.begin() + copy_start_idx;
    // Global pointer to copy input end:
    auto    src_g_end   = global_array.begin() + copy_end_idx;
    // Local pointer to copy input begin, or nullptr if not local:
    ElementType * src_l_begin = src_g_begin.local();

    // ------------------------------------------------------------------------
    // -- Initialize global array: --------------------------------------------
    for (size_t l = 0; l < block_size; ++l) {
      global_array.local[l] = (l+1) * (myid+1)
                              + (std::rand() / RAND_MAX);
    }
    dash::barrier();
    // -- Prevent copying from cache: -----------------------------------------
    if (params.flush_cache && myid == init_unit_id) {
      // Prevent copying from L3 cache by initializing values to be copied on
      // a remote node, i.e. on different node than target unit:
      ElementType * block_values = new ElementType[block_size];
      for (size_t p = 0; p < block_size; ++p) {
        block_values[p] = ((myid+1) * 100000)
                          + (p * 1000);
      }
      // Copy block values to global array:
      dash::copy(block_values,
                 block_values + block_size,
                 src_g_begin);
      // Free local block values after they have been copied to source block:
      delete[] block_values;
    }
    dash::barrier();
    // -- Finished initialization of global array. ----------------------------
    // ------------------------------------------------------------------------

#ifdef DASH_ENABLE_IPM
    MPI_Pcontrol(0, "on");
#endif
    // -- Copy array block from source to destination rank: -------------------
    if(myid == target_unit_id) {
      total_init_us += Timer::ElapsedSince(ts_init_start);
      ElementType * copy_lend = nullptr;

      auto ts_copy_start = Timer::Now();
      if (l_copy_method == STD_COPY) {
          copy_lend = std::copy(src_l_begin,
                                src_l_begin + block_size,
                                local_array);
      } else if (l_copy_method == MEMCPY) {
          copy_lend = local_array + block_size;
          std::memcpy(local_array, src_l_begin, block_size * sizeof(ElementType));
      } else if (l_copy_method == DASH_COPY_ASYNC) {
          copy_lend = dash::copy_async(src_g_begin,
                                       src_g_end,
                                       local_array).get();
      } else {
          copy_lend = dash::copy(src_g_begin,
                                 src_g_end,
                                 local_array);
      }
      auto copy_us   = Timer::ElapsedSince(ts_copy_start);
      total_copy_us += copy_us;
      history_copy_us.push_back(copy_us);

      // -- Finished copy from source to destination rank. --------------------

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
  if (local_array != nullptr) {
    free(local_array);
  }

  if(myid == target_unit_id) {
    time_copy_us.set(total_copy_us);
    time_init_us.set(total_init_us);

    std::sort(history_copy_us.begin(), history_copy_us.end());
    time_copy_med_us.set(history_copy_us[history_copy_us.size() / 2]);
    time_copy_sdv_us.set(dash::math::sigma(history_copy_us.begin(),
                                           history_copy_us.end()));
    time_copy_min_us.set(history_copy_us.front());
    time_copy_max_us.set(history_copy_us.back());

  }

  global_array.deallocate();

  DASH_LOG_DEBUG(
      "copy_block_to_local",
      "Waiting for completion of copy operation");
  dash::barrier();

  double mb_copied   = static_cast<double>(block_bytes * num_repeats)
                       / 1024.0 / 1024.0;

  result.time_init_s      = time_init_us.get() * 1.0e-6;
  result.time_copy_s      = time_copy_us.get() * 1.0e-6;
  result.time_copy_min_us = time_copy_min_us.get();
  result.time_copy_max_us = time_copy_max_us.get();
  result.time_copy_med_us = time_copy_med_us.get();
  result.time_copy_sdv_us = time_copy_sdv_us.get();
  result.mb_per_s         = mb_copied / result.time_copy_s;

  return result;
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw(5)  << "units"       << ","
         << std::setw(9)  << "mpi.impl"    << ","
         << std::setw(10) << "scenario"    << ","
         << std::setw(12) << "copy.type"   << ","
         << std::setw(7)  << "src.u"       << ","
         << std::setw(7)  << "dest.u"      << ","
         << std::setw(7)  << "init.u"      << ","
         << std::setw(8)  << "repeats"     << ","
         << std::setw(9)  << "block.n"     << ","
         << std::setw(9)  << "block.kb"    << ","
         << std::setw(7)  << "init.s"      << ","
         << std::setw(8)  << "copy.s"      << ","
         << std::setw(12) << "copy.min.us" << ","
         << std::setw(12) << "copy.med.us" << ","
         << std::setw(12) << "copy.max.us" << ","
         << std::setw(12) << "copy.sdv.us" << ","
         << std::setw(7)  << "time.s"      << ","
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
    std::string mpi_impl = dash__toxstr(DASH_MPI_IMPL_ID);
    size_t block_n     = size / dash::size();
    size_t g_size_kb   = (size * sizeof(ElementType)) / 1024;
    size_t block_kb    = (block_n * sizeof(ElementType)) / 1024;
    double mbps        = measurement.mb_per_s;
    double init_s      = measurement.time_init_s;
    double copy_s      = measurement.time_copy_s;
    double copy_min_us = measurement.time_copy_min_us;
    double copy_max_us = measurement.time_copy_max_us;
    double copy_med_us = measurement.time_copy_med_us;
    double copy_sdv_us = measurement.time_copy_sdv_us;
    cout << std::right
         << std::setw(5)  << dash::size()            << ","
         << std::setw(9)  << mpi_impl                << ","
         << std::setw(10) << scenario                << ","
         << std::setw(12) << local_copy_method       << ","
         << std::setw(7)  << unit_src                << ","
         << std::setw(7)  << unit_dest               << ","
         << std::setw(7)  << unit_init               << ","
         << std::setw(8)  << num_repeats             << ","
         << std::setw(9)  << block_n                 << ","
         << std::setw(9)  << block_kb                << ","
         << std::fixed << setprecision(2) << setw(7)  << init_s      << ","
         << std::fixed << setprecision(5) << setw(8)  << copy_s      << ","
         << std::fixed << setprecision(2) << setw(12) << copy_min_us << ","
         << std::fixed << setprecision(2) << setw(12) << copy_med_us << ","
         << std::fixed << setprecision(2) << setw(12) << copy_max_us << ","
         << std::fixed << setprecision(2) << setw(12) << copy_sdv_us << ","
         << std::fixed << setprecision(2) << setw(7)  << secs        << ","
         << std::fixed << setprecision(2) << setw(9)  << mbps
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base      = 4;
  params.num_iterations = 8;
  params.rep_base       = params.size_base;
  params.num_repeats    = 0;
  params.min_repeats    = 1;
  params.verify         = false;
  params.local_only     = false;
  params.flush_cache    = false;
  params.size_min       = 64;

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
      params.flush_cache    = true;
      --i;
    }
  }
  if (params.num_repeats == 0) {
    params.num_repeats = 8 * std::pow(params.rep_base, params.num_iterations);
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
  bench_cfg.print_param("-smin",   "initial block size",    params.size_min);
  bench_cfg.print_param("-sb",     "block size base",       params.size_base);
  bench_cfg.print_param("-rmax",   "initial repeats",       params.num_repeats);
  bench_cfg.print_param("-rmin",   "min, repeats",          params.min_repeats);
  bench_cfg.print_param("-rb",     "rep. base",             params.rep_base);
  bench_cfg.print_param("-i",      "iterations",            params.num_iterations);
  bench_cfg.print_param("-verify", "verification",          params.verify);
  bench_cfg.print_param("-lo",     "local only",            params.local_only);
  bench_cfg.print_param("-fcache", "no copying from cache", params.flush_cache);
  bench_cfg.print_section_end();
}


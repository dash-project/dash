#include <libdash.h>

#include <iostream>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::setprecision;

using dash::util::BenchmarkParams;

#define LOAD_BALANCE

// ==========================================================================
// Type definitions
// ==========================================================================

typedef int
  ElementType;
typedef dash::default_index_t
  IndexType;

#ifdef LOAD_BALANCE
typedef dash::LoadBalancePattern<1>
  PatternType;
#else
typedef dash::Pattern<1>
  PatternType;
#endif

typedef dash::Array<ElementType, IndexType, PatternType>
  ArrayType;
typedef dash::util::Timer<dash::util::TimeMeasure::Clock>
  Timer;
typedef typename dash::util::BenchmarkParams::config_params_type
  bench_cfg_params;

typedef struct benchmark_params_t
  {
    size_t size_base;
    size_t size_min;
    size_t num_iterations;
    size_t num_repeats;
    size_t min_repeats;
    size_t rep_base;
  }
  benchmark_params;

typedef struct measurement_t
  {
    double time_s;
    double time_min_us;
    double time_max_us;
    double time_med_us;
    double time_sdv_us;
    double mkeys_per_s;
  }
  measurement;

// ==========================================================================
// Function Declarations
// ==========================================================================

measurement perform_test(
  size_t NELEM,
  size_t REPEAT);

void print_measurement_header();
void print_measurement_record(
  const bench_cfg_params & cfg_params,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  measurement              measurement,
  const benchmark_params & params);

benchmark_params parse_args(
  int    argc,
  char * argv[]);

void print_params(
  const BenchmarkParams  & bench_cfg,
  const benchmark_params & params);

void print_local_sizes(
  const BenchmarkParams  & bench_cfg,
  const PatternType      & pattern);

// ==========================================================================
// Benchmark Implementation
// ==========================================================================

int main(int argc, char **argv)
{
  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  measurement res;
  double time_s;
  auto   ts_start         = Timer::Now();

  benchmark_params params = parse_args(argc, argv);
  size_t num_iterations   = params.num_iterations;
  size_t num_repeats      = params.num_repeats;
  size_t size_inc         = params.size_min;

  dash::util::BenchmarkParams bench_params("bench.08.min-element");
  bench_params.print_header();
  bench_params.print_pinning();

  auto   bench_cfg        = bench_params.config();

#ifdef LOAD_BALANCE
  dash::util::TeamLocality tloc(dash::Team::All());
  PatternType pattern(
    dash::SizeSpec<1>(size_inc),
    tloc);
#else
  PatternType pattern(size_inc);
#endif

  print_params(bench_params, params);
  print_local_sizes(bench_params, pattern);

  print_measurement_header();

  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    size_t size = std::pow(params.size_base, i) * size_inc;

    num_repeats = std::max<size_t>(num_repeats, params.min_repeats);

    ts_start = Timer::Now();
    res      = perform_test(size, num_repeats);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;
    print_measurement_record(bench_cfg, size, num_repeats,
                             time_s, res, params);
  }

  dash::finalize();

  return 0;
}

measurement perform_test(
  size_t NELEM,
  size_t REPEAT)
{
  measurement result;
  result.time_s        = 0;
  result.time_min_us   = 0;
  result.time_max_us   = 0;
  result.mkeys_per_s   = 0;

  auto myid = dash::myid();

  // Total time:
  dash::Shared<double> time_us;
  // Minimum duration for a single operation:
  dash::Shared<double> time_min_us;
  // Maximum duration for a single operation:
  dash::Shared<double> time_max_us;
  // Median of duration of operations:
  dash::Shared<double> time_med_us;
  // Standard deviation of durations:
  dash::Shared<double> time_sdv_us;

  double duration_us;

#ifdef LOAD_BALANCE
  dash::util::TeamLocality tloc(dash::Team::All());
  PatternType pattern(
    dash::SizeSpec<1>(NELEM),
    tloc);
#else
  PatternType pattern(NELEM);
#endif

  ArrayType arr(pattern);

  for (auto & el: arr.local) {
    el = rand();
  }
  arr.barrier();

  double total_time_us = 0;
  std::vector<double> history_time_us;
  for (size_t i = 0; i < REPEAT; i++) {
    dash::barrier();

    auto ts_start = Timer::Now();
    auto min = dash::min_element(arr.begin(), arr.end());
    dash__unused(min);

    auto time_us   = Timer::ElapsedSince(ts_start);
    total_time_us += time_us;
    history_time_us.push_back(time_us);
  }

  if(myid == 0) {
    time_us.set(total_time_us);

    std::sort(history_time_us.begin(), history_time_us.end());
    time_med_us.set(history_time_us[history_time_us.size() / 2]);
    time_sdv_us.set(dash::math::sigma(history_time_us.begin(),
                                      history_time_us.end()));
    time_min_us.set(history_time_us.front());
    time_max_us.set(history_time_us.back());
  }

  DASH_LOG_DEBUG("perform_test", "Waiting for completion of all units");
  dash::barrier();

  double mkeys       = static_cast<double>(NELEM * REPEAT)
                       / 1024.0 / 1024.0;

  result.time_s      = time_us.get() * 1.0e-6;
  result.time_min_us = time_min_us.get();
  result.time_max_us = time_max_us.get();
  result.time_med_us = time_med_us.get();
  result.time_sdv_us = time_sdv_us.get();
  result.mkeys_per_s = mkeys / result.time_s;

  return result;
}

// ==========================================================================
// Auxiliary Functions
// ==========================================================================

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw(5)  << "units"       << ","
         << std::setw(9)  << "mpi.impl"    << ","
         << std::setw(8)  << "repeats"     << ","
         << std::setw(12) << "size"        << ","
         << std::setw(12) << "time.s"      << ","
         << std::setw(12) << "time.min.us" << ","
         << std::setw(12) << "time.med.us" << ","
         << std::setw(12) << "time.max.us" << ","
         << std::setw(12) << "time.sdv.us" << ","
         << std::setw(12) << "total.s"     << ","
         << std::setw(9)  << "mkeys/s"
         << endl;
  }
}

void print_measurement_record(
  const bench_cfg_params & cfg_params,
  size_t                   size,
  int                      num_repeats,
  double                   secs,
  measurement              measurement,
  const benchmark_params & params)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
    double mkps        = measurement.mkeys_per_s;
    double time_s      = measurement.time_s;
    double time_min_us = measurement.time_min_us;
    double time_max_us = measurement.time_max_us;
    double time_med_us = measurement.time_med_us;
    double time_sdv_us = measurement.time_sdv_us;
    cout << std::right
         << std::setw(5)  << dash::size() << ","
         << std::setw(9)  << mpi_impl     << ","
         << std::setw(8)  << num_repeats  << ","
         << std::setw(12) << size         << ","
         << std::fixed << setprecision(2) << setw(12) << time_s      << ","
         << std::fixed << setprecision(2) << setw(12) << time_min_us << ","
         << std::fixed << setprecision(2) << setw(12) << time_med_us << ","
         << std::fixed << setprecision(2) << setw(12) << time_max_us << ","
         << std::fixed << setprecision(2) << setw(12) << time_sdv_us << ","
         << std::fixed << setprecision(2) << setw(12) << secs        << ","
         << std::fixed << setprecision(2) << setw(9)  << mkps
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base      = 2;
  params.num_iterations = 4;
  params.rep_base       = 2;
  params.num_repeats    = 0;
  params.min_repeats    = 1;
  params.size_min       = 1024;

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
    }
  }
  if (params.num_repeats == 0) {
    params.num_repeats = params.size_min *
                         std::pow(params.rep_base, params.num_iterations);
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
  bench_cfg.print_param("-smin",   "initial size",    params.size_min);
  bench_cfg.print_param("-sb",     "size base",       params.size_base);
  bench_cfg.print_param("-rmax",   "initial repeats", params.num_repeats);
  bench_cfg.print_param("-rmin",   "min. repeats",    params.min_repeats);
  bench_cfg.print_param("-rb",     "rep. base",       params.rep_base);
  bench_cfg.print_param("-i",      "iterations",      params.num_iterations);
  bench_cfg.print_section_end();
}

void print_local_sizes(
  const dash::util::BenchmarkParams & bench_cfg,
  const PatternType                 & pattern)
{
  bench_cfg.print_section_start("Data Partitioning");
  for (size_t u = 0; u < pattern.team().size(); u++) {
    std::ostringstream uss;
    uss << "unit " << setw(2) << u;
    bench_cfg.print_param(uss.str(), "local size", pattern.local_size(u));
  }
  bench_cfg.print_section_end();
}

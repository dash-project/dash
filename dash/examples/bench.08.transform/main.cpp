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

typedef double
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
    bool   verify;
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
  size_t                   NELEM,
  size_t                   REPEAT,
  const benchmark_params & params);

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

void print_team_locality(
  const dash::util::BenchmarkParams & bench_cfg,
  const dash::util::TeamLocality    & tloc);

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

  dash::util::BenchmarkParams bench_params("bench.08.transform");
  bench_params.print_header();
  bench_params.print_pinning();

  auto   bench_cfg        = bench_params.config();

  dash::util::TeamLocality tloc(dash::Team::All());

#ifdef LOAD_BALANCE
  PatternType pattern(
    dash::SizeSpec<1>(size_inc),
    tloc);
#else
  PatternType pattern(size_inc);
#endif

  dash::barrier();

  print_params(bench_params, params);
  print_local_sizes(bench_params, pattern);
  print_team_locality(bench_params, tloc);

  print_measurement_header();

  dash::barrier();

  num_repeats = params.num_repeats;
  for (size_t i = 0; i < num_iterations && num_repeats > 0;
       ++i, num_repeats /= params.rep_base)
  {
    dash::barrier();

    size_t size = std::pow(params.size_base, i) * size_inc;

    num_repeats = std::max<size_t>(num_repeats, params.min_repeats);

    dash::util::TraceStore::on();
    dash::util::TraceStore::clear();

    ts_start = Timer::Now();
    res      = perform_test(size, num_repeats, params);
    time_s   = Timer::ElapsedSince(ts_start) * 1.0e-06;

    dash::barrier();

    std::ostringstream ss;
    ss << "transform.iteration-" << i;

    dash::util::TraceStore::write(ss.str());
    dash::util::TraceStore::clear();
    dash::util::TraceStore::off();

    print_measurement_record(bench_cfg, size, num_repeats,
                             time_s, res, params);
  }

  dash::finalize();

  return 0;
}

measurement perform_test(
  size_t                   NELEM,
  size_t                   REPEAT,
  const benchmark_params & params)
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

  dash::Shared<IndexType> min_lidx_exp;

  double duration_us;

#ifdef LOAD_BALANCE
  dash::util::TeamLocality tloc(dash::Team::All());
  PatternType pattern(
    dash::SizeSpec<1>(NELEM),
    tloc);
#else
  PatternType pattern(NELEM);
#endif

  ArrayType arr_a(pattern);
  ArrayType arr_b(pattern);
  ArrayType arr_c(pattern);

  for (size_t li = 0; li < arr_a.lsize(); li++) {
    arr_a.local[li] = 1 + ((42 * (li + 1)) % 1024);
    arr_b.local[li] = 1 + ((42 * (li + 1)) % 1024);
  }

  dash::barrier();

  dash::util::TraceStore::off();

  double total_time_us = 0;
  std::vector<double> history_time_us;
  for (size_t i = 0; i < REPEAT; i++) {
    dash::barrier();

    if (REPEAT == 1 || i == 1) {
      dash::util::TraceStore::on();
    }

    auto ts_start = Timer::Now();
    auto min_git  = dash::transform(arr_a.begin(), arr_a.end(),
                                    arr_b.begin(),
                                    arr_c.begin(),
                                    dash::plus<ElementType>());
    auto time_us  = Timer::ElapsedSince(ts_start);

    if (REPEAT == 1 || i == 1) {
      dash::util::TraceStore::off();
    }

    total_time_us += time_us;
    history_time_us.push_back(time_us);

    if (params.verify) {
      // Test first local 1000 values only:
      for (size_t li = 0; li < std::min<size_t>(1000, arr_a.lsize()); li++) {
        auto expected = arr_a.local[li] + arr_b.local[li];
        auto actual   = arr_c.local[li];
        if (actual != expected) {
          std::cout <<
            "dash::transform: " <<
            "expected (" << expected << ") != " <<
            "actual ("   << expected << ")"     <<
            "at unit:"   << dash::myid() << " " <<
            "lidx:"      << li           << " " <<
            "in repeat " << i
            << std::endl;
          break;
        }
      }
    }

    dash::barrier();
  }

  if (myid == 0) {
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
    std::ostringstream oss;
    oss << std::right
        << std::setw(5)  << "units"       << ","
        << std::setw(9)  << "mpi.impl"    << ","
        << std::setw(8)  << "repeats"     << ","
        << std::setw(11) << "size"        << ","
        << std::setw( 8) << "time.s"      << ","
        << std::setw(12) << "time.min.us" << ","
        << std::setw(12) << "time.med.us" << ","
        << std::setw(12) << "time.max.us" << ","
        << std::setw(12) << "time.sdv.us" << ","
        << std::setw( 8) << "total.s"     << ","
        << std::setw(10) << "mkeys/s"
        << endl;
    std::cout << oss.str();
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
  if (dash::myid() != 0) {
    return;
  }

  std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
  double mkps        = measurement.mkeys_per_s;
  double time_s      = measurement.time_s;
  double time_min_us = measurement.time_min_us;
  double time_max_us = measurement.time_max_us;
  double time_med_us = measurement.time_med_us;
  double time_sdv_us = measurement.time_sdv_us;

  std::ostringstream oss;
  oss << std::right
      << std::setw(5)  << dash::size() << ","
      << std::setw(9)  << mpi_impl     << ","
      << std::setw(8)  << num_repeats  << ","
      << std::setw(11) << size         << ","
      << std::fixed << setprecision(2) << setw( 8) << time_s      << ","
      << std::fixed << setprecision(2) << setw(12) << time_min_us << ","
      << std::fixed << setprecision(2) << setw(12) << time_med_us << ","
      << std::fixed << setprecision(2) << setw(12) << time_max_us << ","
      << std::fixed << setprecision(2) << setw(12) << time_sdv_us << ","
      << std::fixed << setprecision(2) << setw( 8) << secs        << ","
      << std::fixed << setprecision(2) << setw(10) << mkps
      << endl;
  std::cout << oss.str();
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base      = 2;
  params.num_iterations = 8;
  params.rep_base       = 2;
  params.num_repeats    = 0;
  params.min_repeats    = 10;
  params.size_min       = 8.0e+6; // 800k elements
  params.verify         = false;

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
    } else if (flag == "-v") {
      params.verify         = true;
      i++;
    }
  }
  if (params.num_repeats == 0) {
    params.num_repeats = params.min_repeats *
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
  bench_cfg.print_param("-v",      "verify",          params.verify);
  bench_cfg.print_section_end();
}

void print_local_sizes(
  const dash::util::BenchmarkParams & bench_cfg,
  const PatternType                 & pattern)
{
  if (dash::myid() != 0) {
    return;
  }

  bench_cfg.print_section_start("Data Partitioning");
  bench_cfg.print_param("global", "cpu  mbw  ldw", pattern.size());
  for (dash::team_unit_t u{0}; u < pattern.team().size(); u++) {
    std::ostringstream uss;
    uss << "u:" << setw(4) << u;

    std::ostringstream lss;
#ifdef LOAD_BALANCE
    double cpu_weight   = pattern.unit_cpu_weights()[u];
    double membw_weight = pattern.unit_membw_weights()[u];
    double load_weight  = pattern.unit_load_weights()[u];
    lss << std::fixed << std::setprecision(2) << cpu_weight   << " "
        << std::fixed << std::setprecision(2) << membw_weight << " "
        << std::fixed << std::setprecision(2) << load_weight;
#else
    lss << 1.0;
#endif

    bench_cfg.print_param(uss.str(), lss.str(), pattern.local_size(u));
  }
  bench_cfg.print_section_end();
}

void print_team_locality(
  const dash::util::BenchmarkParams & bench_cfg,
  const dash::util::TeamLocality    & tloc)
{
  if (dash::myid() != 0) {
    return;
  }

  std::stringstream ss;
  ss << tloc.domain();

  bench_cfg.print_section_start("Team Locality Domains");
  bench_cfg.print(ss, "#");
  bench_cfg.print_section_end();
}

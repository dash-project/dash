/**
 * Measures the performance of different
 * for_each implementations on dash containers
 */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#ifdef MPI_IMPL_ID
#include <mpi.h>
#endif

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

typedef typename dash::util::BenchmarkParams::config_params_type
  bench_cfg_params;

using TeamSpecT = dash::TeamSpec<2>;
using ValueT    = double;
using MatrixT   = dash::NArray<ValueT, 2>;
using PatternT  = typename MatrixT::pattern_type;
using SizeSpecT = dash::SizeSpec<2>;
using DistSpecT = dash::DistributionSpec<2>;

typedef struct benchmark_params_t {

  benchmark_params_t()
  { }
  int    reps        = 10;
  int    rounds      = 10;
  size_t matrix_ext  = 1024;
  size_t tile_ext    = 64;
} benchmark_params;

typedef struct measurement_t {
  std::string testcase;
  double      time_total_s;
} measurement;

#ifdef HAVE_ASSERT
#include <cassert>
#define ASSERT_EQ(_e, _a) do {  \
  assert((_e) == (_a));         \
} while (0)
#else
#define ASSERT_EQ(_e, _a) do {  \
  dash__unused(_e);             \
  dash__unused(_a);             \
} while (0)
#endif

void print_measurement_header();
void print_measurement_record(
  const bench_cfg_params & cfg_params,
  std::string              name,
  double                   time_in_s,
  double                   bandwidth);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);

template<bool UseHandles>
double evaluate(
  int reps, MatrixT& matrix, ValueT *buffer,
  benchmark_params params);

double evaluate_async(
  int reps, MatrixT& matrix, ValueT *buffer,
  benchmark_params params);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  dash::util::BenchmarkParams bench_params("bench.15.copy");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  size_t matrix_ext = params.matrix_ext;
  size_t tile_ext   = params.tile_ext;


  auto& team_all = dash::Team::All();
  TeamSpecT team_all_spec(team_all.size(), 1);
  team_all_spec.balance_extents();

  auto size_spec = SizeSpecT(matrix_ext, matrix_ext);
  auto dist_spec = DistSpecT(dash::TILE(tile_ext), dash::TILE(tile_ext));

  MatrixT matrix(size_spec, dist_spec, team_all, team_all_spec);

  print_params(bench_params, params);
  print_measurement_header();

  int          round = 0;

  ValueT *buffer = new ValueT[matrix.size()];

  size_t matrix_size_b = matrix.size() * sizeof(ValueT);

  while(round < params.rounds) {
    double res;
    res = evaluate<true>(params.reps, matrix, buffer, params);
    print_measurement_record(bench_cfg, "copy_with_handle",
                             res, matrix_size_b / res / 1E6);

    res = evaluate<false>(params.reps, matrix, buffer, params);
    print_measurement_record(bench_cfg, "copy_without_handle",
                             res, matrix_size_b / res / 1E6);

    res = evaluate_async(params.reps, matrix, buffer, params);
    print_measurement_record(bench_cfg, "copy_async",
                             res, matrix_size_b / res / 1E6);
    round++;
  }

  if (dash::myid() == 0) {
    cout << "Benchmark finished" << endl;
  }

  dash::finalize();
  return 0;
}

template<bool UseHandles>
double evaluate(
  int reps, MatrixT& matrix, ValueT *buffer,
  benchmark_params params)
{
  measurement mes;

  auto r = dash::myid();

  auto begin = matrix.begin();
  auto end   = matrix.end();

  float lmin = r;
  float lmax = 1000 - r;

  auto ts_tot_start = Timer::Now();

  for (int i = 0; i < reps; i++) {
    if (dash::myid() == 0) {
      dash::copy<ValueT, decltype(begin), UseHandles>(begin, end, buffer);
    }
  }
  dash::barrier();

  return Timer::ElapsedSince(ts_tot_start) / (double)reps / 1E6;
}


double evaluate_async(
  int reps, MatrixT& matrix, ValueT *buffer,
  benchmark_params params)
{
  measurement mes;

  auto r = dash::myid();

  auto begin = matrix.begin();
  auto end   = matrix.end();

  float lmin = r;
  float lmax = 1000 - r;

  auto ts_tot_start = Timer::Now();

  for (int i = 0; i < reps; i++) {
    if (dash::myid() == 0) {
      auto fut = dash::copy_async(begin, end, buffer);
      fut.wait();
    }
  }
  dash::barrier();

  return Timer::ElapsedSince(ts_tot_start) / (double)reps / 1E6;
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw( 5) << "units"      << ","
         << std::setw( 9) << "mpi.impl"   << ","
         << std::setw(30) << "impl"       << ","
         << std::setw(12) << "total [s]"    << ","
         << std::setw(20) << "bandwidth [MB/s]"
         << endl;
  }
}

void print_measurement_record(
  const bench_cfg_params & cfg_params,
  std::string              name,
  double                   time_in_s,
  double                   bandwidth)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(DASH_MPI_IMPL_ID);
    cout << std::right
         << std::setw(5) << dash::size() << ","
         << std::setw(9) << mpi_impl     << ","
         << std::fixed << setprecision(2) << setw(30) << name << ","
         << std::fixed << setprecision(8) << setw(12) << time_in_s << ","
         << std::fixed << setprecision(8) << setw(20) << bandwidth
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-r") {
      params.reps = atoi(argv[i+1]);
    }
    if (flag == "-n") {
      params.rounds = atoi(argv[i+1]);
    }
    if (flag == "-t") {
      params.tile_ext = atoi(argv[i+1]);
    }
    if (flag == "-s") {
      params.matrix_ext = atoi(argv[i+1]);
    }
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
  bench_cfg.print_param("-r", "repetitions per round", params.reps);
  bench_cfg.print_param("-n", "rounds",                params.rounds);
  bench_cfg.print_param("-s",
                        "matrix size (number of double elements per dimension)",
                        params.matrix_ext);
  bench_cfg.print_param("-t",
                        "tile size (number of double elements per dimension)",
                        params.tile_ext);
  bench_cfg.print_section_end();
}

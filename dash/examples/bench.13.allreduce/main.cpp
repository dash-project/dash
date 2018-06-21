/**
 * Measures the performance of different
 * for_each implementations on dash containers
 */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <string>

using std::cout;
using std::endl;
using std::setw;
using std::setprecision;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

typedef typename dash::util::BenchmarkParams::config_params_type
  bench_cfg_params;

typedef struct benchmark_params_t {
  int    reps;
  int    rounds;
} benchmark_params;

typedef struct measurement_t {
  std::string testcase;
  double      time_total_s;
} measurement;

void print_measurement_header();
void print_measurement_record(
  const bench_cfg_params & cfg_params,
  measurement              measurement,
  const benchmark_params & params);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);


measurement evaluate(
              int reps,
              std::string testcase,
              benchmark_params params);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  measurement res;

  dash::util::BenchmarkParams bench_params("bench.13.allreduce");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);
  print_measurement_header();

  int     multiplier = 1;
  int          round = 0;
  std::array<std::string, 3> testcases {{
                            "dart_allreduce.minmax",
                            "dart_allreduce.min",
                            "dart_allreduce.shared"
                            }};

  while(round < params.rounds) {
    auto time_start = Timer::Now();
    for(auto testcase : testcases){
      res = evaluate(params.reps, testcase, params);
      print_measurement_record(bench_cfg, res, params);
    }
    round++;
  }

  if (dash::myid() == 0) {
    cout << "Benchmark finished" << endl;
  }

  dash::finalize();
  return 0;
}

measurement evaluate(int reps, std::string testcase, benchmark_params params)
{
  measurement mes;

  auto r = dash::myid();


  float lmin = r;
  float lmax = 1000000 - r;

  auto ts_tot_start = Timer::Now();

  for (int i = 0; i < reps; i++) {
    if (testcase == "dart_allreduce.minmax") {
      std::array<float, 2> min_max_in{lmin, lmax};
      std::array<float, 2> min_max_out{};
      dart_allreduce(
          &min_max_in,                        // send buffer
          &min_max_out,                       // receive buffer
          2,                                  // buffer size
          dash::dart_datatype<float>::value,  // data type
          DART_OP_MINMAX,                     // operation
          dash::Team::All().dart_id()         // team
          );
    }
    else if (testcase == "dart_allreduce.min") {
      std::array<float, 2> min_max_in{lmin, lmax};
      std::array<float, 2> min_max_out{};
      dart_allreduce(
          &min_max_in,                        // send buffer
          &min_max_out,                       // receive buffer
          1,                                  // buffer size
          dash::dart_datatype<float>::value,  // data type
          DART_OP_MIN,                        // operation
          dash::Team::All().dart_id()         // team
          );
      dart_allreduce(
          &min_max_in[1],                     // send buffer
          &min_max_out[1],                    // receive buffer
          1,                                  // buffer size
          dash::dart_datatype<float>::value,  // data type
          DART_OP_MAX,                        // operation
          dash::Team::All().dart_id()         // team
          );
    }
    else if (testcase == "dart_allreduce.shared") {
      using value_t = float;
      using shared_t = dash::Shared<dash::Atomic<float>>;

      auto&    team = dash::Team::All();
      shared_t g_min(std::numeric_limits<value_t>::max(), dash::team_unit_t{0}, team);
      shared_t g_max(std::numeric_limits<value_t>::min(), dash::team_unit_t{0}, team);

      auto const start_min = static_cast<value_t>(g_min.get());
      auto const start_max = static_cast<value_t>(g_max.get());

      team.barrier();

      g_min.get().fetch_op(dash::min<value_t>(), 0);
      g_max.get().fetch_op(dash::max<value_t>(), std::numeric_limits<value_t>::max());

      team.barrier();
    }
  }

  mes.time_total_s   = Timer::ElapsedSince(ts_tot_start) / (1000 * 1000);
  mes.testcase       = testcase;
  return mes;
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw( 5) << "units"      << ","
         << std::setw( 9) << "mpi.impl"   << ","
         << std::setw(30) << "impl"       << ","
         << std::setw( 8) << "total.s"
         << endl;
  }
}

void print_measurement_record(
  const bench_cfg_params & cfg_params,
  measurement              measurement,
  const benchmark_params & params)
{
  if (dash::myid() == 0) {
    std::string mpi_impl = dash__toxstr(DASH_MPI_IMPL_ID);
    auto mes = measurement;
        cout << std::right
         << std::setw(5) << dash::size() << ","
         << std::setw(9) << mpi_impl     << ","
         << std::fixed << setprecision(2) << setw(30) << mes.testcase       << ","
         << std::fixed << setprecision(8) << setw(12) << mes.time_total_s
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.reps           = 100;
  params.rounds         = 10;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-r") {
      params.reps = atoi(argv[i+1]);
    }
    if (flag == "-n") {
      params.rounds = atoi(argv[i+1]);
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
  bench_cfg.print_param("-r",    "repetitions per round", params.reps);
  bench_cfg.print_param("-n",    "rounds", params.rounds);
  bench_cfg.print_section_end();
}

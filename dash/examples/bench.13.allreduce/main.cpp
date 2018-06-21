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

typedef struct {
  float min, max;
} minmax_t;

static void minmax_fn(const void *invec, void *inoutvec, size_t, void *)
{
  const minmax_t* minmax_in = static_cast<const minmax_t*>(invec);
  minmax_t* minmax_out = static_cast<minmax_t*>(inoutvec);

  if (minmax_in->min < minmax_out->min) {
    minmax_out->min = minmax_in->min;
  }

  if (minmax_in->max > minmax_out->max) {
    minmax_out->max = minmax_in->max;
  }
}

template<typename T>
static void minmax_lambda(const void *invec, void *inoutvec, size_t, void *userdata)
{
  const minmax_t* minmax_in = static_cast<const minmax_t*>(invec);
  minmax_t* minmax_out = static_cast<minmax_t*>(inoutvec);
  T& fn = *static_cast<T*>(userdata);
  *minmax_out = fn(*minmax_in, *minmax_out);
}

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
  std::array<std::string, 5> testcases {{
                            "dart_allreduce.minmax",
                            "dart_allreduce.min",
                            "dart_allreduce.shared",
                            "dart_allreduce.custom",
                            "dart_allreduce.lambda"
                            }};

  while(round < params.rounds) {
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
    } else if (testcase == "dart_allreduce.custom") {
      minmax_t min_max_in{lmin, lmax};
      minmax_t min_max_out{};
      dart_datatype_t  new_type;
      dart_operation_t new_op;
      dart_type_create_custom(sizeof(minmax_t), &new_type);
      dart_op_create(
        &minmax_fn, NULL, true, new_type, true, &new_op);
      dart_allreduce(
          &min_max_in,                        // send buffer
          &min_max_out,                       // receive buffer
          1,                                  // buffer size
          new_type,                           // data type
          new_op,                             // operation
          dash::Team::All().dart_id()         // team
          );
      dart_type_destroy(&new_type);
      dart_op_destroy(&new_op);
    } else if (testcase == "dart_allreduce.lambda") {
      minmax_t min_max_in{lmin, lmax};
      minmax_t min_max_out{};
      dart_datatype_t  new_type;
      dart_operation_t new_op;
      dart_type_create_custom(sizeof(minmax_t), &new_type);
      auto fn = [](const minmax_t &a, const minmax_t &b){
        minmax_t res = b;
        if (a.min < res.min) {
          res.min = a.min;
        }
        if (a.max > res.max) {
          res.max = a.max;
        }
        return res;
      };
      dart_op_create(
        &minmax_lambda<decltype(fn)>, &fn, true, new_type, true, &new_op);
      dart_allreduce(
          &min_max_in,                        // send buffer
          &min_max_out,                       // receive buffer
          1,                                  // buffer size
          new_type,                           // data type
          new_op,                             // operation
          dash::Team::All().dart_id()         // team
          );
      dart_type_destroy(&new_type);
      dart_op_destroy(&new_op);
    }
  }

  mes.time_total_s   = Timer::ElapsedSince(ts_tot_start) / (double)reps / 1E6;
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
    std::string mpi_impl = dash__toxstr(MPI_IMPL_ID);
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

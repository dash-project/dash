/**
 * Measures the performance of different
 * for_each implementations on dash containers
 */

#include <libdash.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <mpi.h>

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

enum experiment_t{
  ARRAYSTRUCT = 0,
  ARRAYDOUBLE,
  DARTSTRUCT,
  DARTDOUBLE,
  DARTLAMBDA,
  MPIDOUBLE
};

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

std::array<const char*, 6> testcase_str {{
                          "accumulate.arraystruct",
                          "accumulate.arraydouble",
                          "accumulate.dartstruct",
                          "accumulate.dartdouble",
                          "accumulate.dartlambda",
                          "accumulate.mpidouble"
                          }};


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
              experiment_t     testcase,
              benchmark_params params);

template<typename ValueType, typename BinaryOperation>
ValueType accumulate_array(
  const ValueType* l_first,
  const ValueType* l_last,
  const ValueType  init,
  BinaryOperation binary_op = dash::plus<ValueType>())
{
  struct local_result {
    ValueType l_result;
    bool      l_valid;
  };

  auto & team      = dash::Team::All();
  auto myid        = team.myid();
  auto result      = init;

  dash::Array<local_result> l_results(team.size(), team);
  if (l_first != l_last) {
    auto l_result  = std::accumulate(std::next(l_first), l_last, *l_first, binary_op);
    l_results.local[0].l_result = l_result;
    l_results.local[0].l_valid  = true;
  } else {
    l_results.local[0].l_valid  = false;
  }

  l_results.barrier();

  if (myid == 0) {
    for (size_t i = 0; i < team.size(); i++) {
      local_result lr = l_results[i];
      if (lr.l_valid) {
        result = binary_op(result, lr.l_result);
      }
    }
  }
  return result;
}

typedef struct minmax{
  float min, max;
  struct minmax operator+(const minmax &other) const {
    auto res = *this;
    res.min += other.min;
    res.max += other.max;
    return res;
  }

  struct minmax& operator+=(const minmax &other) {
    auto& res = *this;
    res.min += other.min;
    res.max += other.max;
    return res;
  }
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

  dash::util::BenchmarkParams bench_params("bench.14.accumulate");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);
  print_measurement_header();

  int          round = 0;
  std::array<experiment_t, 6> testcases{{
    ARRAYSTRUCT,
    ARRAYDOUBLE,
    DARTSTRUCT,
    DARTDOUBLE,
    DARTLAMBDA,
    MPIDOUBLE
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

measurement evaluate(int reps, experiment_t testcase, benchmark_params params)
{
  measurement mes;

  auto r = dash::myid();


  float lmin = r;
  float lmax = 1000 - r;

  auto ts_tot_start = Timer::Now();

  for (int i = 0; i < reps; i++) {
    if (testcase == ARRAYSTRUCT) {
      minmax_t in{lmin, lmax};
      minmax_t init{0.0, 0.0};
      minmax_t out = accumulate_array(&in, std::next(&in), init,
                                      dash::plus<minmax_t>());
      if (dash::myid() == 0) {
        assert((int)out.min == (dash::size()-1)*(dash::size())/2);
        assert((int)out.max == dash::size()*1000
                                - ((dash::size()-1)*(dash::size()))/2);
      }
    }
    else if (testcase == ARRAYDOUBLE) {
      double in = lmin + lmax;
      double out = accumulate_array(&in, std::next(&in), 0.0,
                                    dash::plus<double>());
      if (dash::myid() == 0) {
        ASSERT_EQ((int)out, (dash::size()-1)*(dash::size())/2 + dash::size()*1000 - ((dash::size()-1)*(dash::size()))/2);
      }
    }
    else if (testcase == DARTSTRUCT) {
      minmax_t in{lmin, lmax};
      minmax_t init{0.0, 0.0};
      minmax_t out = dash::accumulate(&in, std::next(&in), init, true);
      ASSERT_EQ((int)out.min, (dash::size()-1)*(dash::size())/2);
      ASSERT_EQ((int)out.max, dash::size()*1000 - ((dash::size()-1)*(dash::size()))/2);
    } else if (testcase == DARTDOUBLE) {
      double in = lmin + lmax;
      double out = dash::accumulate(&in, std::next(&in), 0.0, true);
      ASSERT_EQ((int)out, (dash::size()-1)*(dash::size())/2 + dash::size()*1000 - ((dash::size()-1)*(dash::size()))/2);
    } else if (testcase == DARTLAMBDA) {
      double in = lmin + lmax;
      double out = dash::accumulate(&in, std::next(&in), 0.0,
                                    [](double a, double b){ return a + b; });
      ASSERT_EQ((int)out, (dash::size()-1)*(dash::size())/2 + dash::size()*1000 - ((dash::size()-1)*(dash::size()))/2);
    } else if (testcase == MPIDOUBLE) {
      double in = lmin + lmax;
      double out;
      MPI_Allreduce(&in, &out, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      ASSERT_EQ((int)out, (dash::size()-1)*(dash::size())/2 + dash::size()*1000 - ((dash::size()-1)*(dash::size()))/2);
    }
  }

  mes.time_total_s   = Timer::ElapsedSince(ts_tot_start) / (double)reps / 1E6;
  mes.testcase       = testcase_str[testcase];
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

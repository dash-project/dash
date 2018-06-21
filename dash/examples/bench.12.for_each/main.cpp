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
  long   size_base;
  int    max_time;
} benchmark_params;

typedef struct measurement_t {
  std::string testcase;
  double      local_elems_s;
  double      local_size_mb;
  double      time_fill_s;
  double      time_foreach_s;
  double      time_total_s;
  bool        uses_local_ptr;
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
              long size,
              std::string testcase,
              benchmark_params params);

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);

  // 0: real, 1: virt
  Timer::Calibrate(0);

  measurement res;

  dash::util::BenchmarkParams bench_params("bench.12.for_each");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();

  print_params(bench_params, params);
  print_measurement_header();

  int     multiplier = 1;
  double  round_time = 0;
  std::array<std::string, 3> testcases {{
                            "std::for_each.l",
                            "dash::for_each.g",
                            "dash::for_each_with_index.g" }};
  // Get locality information
  long num_nodes          = dash::util::Locality::NumNodes();
  long mb_per_node        = 8; // Intermediate, should be
                               //   dash::util::Locality::SystemMemory();
  long global_avail_bytes = num_nodes * mb_per_node * 1024 * 1024;
  long global_req_bytes   = params.size_base * params.size_base * sizeof(int);

  std::cout << "#nodes:   " << num_nodes   << std::endl
            << "node mem: " << mb_per_node << " MiB" << std::endl
            << std::endl;

  while((round_time < params.max_time) &&
        (global_avail_bytes > global_req_bytes)) {
    auto time_start = Timer::Now();
    for(auto testcase : testcases){
      res = evaluate(params.size_base*multiplier, testcase, params);
      print_measurement_record(bench_cfg, res, params);
    }
    multiplier *= 2;
    round_time = Timer::ElapsedSince(time_start) / (1000 * 1000);
    global_req_bytes = (params.size_base * params.size_base) *
                      multiplier * sizeof(int);
  }

  if (dash::myid() == 0) {
    cout << "Benchmark finished" << endl;
  }

  dash::finalize();
  return 0;
}

measurement evaluate(long size, std::string testcase, benchmark_params params)
{
  measurement mes;
  long sum = 0;
  dash::NArray<int, 2> container(size, size);
  auto begin  = container.begin();
  auto lbegin = container.lbegin();
  auto end    = container.end();
  auto lend   = container.lend();
  auto lsize  = container.local.size();

  // inefficient sum
  std::function<void(const int &)> for_each =
      [&sum](int el){
        sum+=el;
      };
  std::function<void(const int &, long)> for_each_index =
    [&sum](int el, long idx) {
      sum+=el;
    };

  dash::barrier();
  auto ts_tot_start = Timer::Now();
  dash::fill(begin, end, 1);
  mes.time_fill_s = Timer::ElapsedSince(ts_tot_start) / (1000 * 1000);

  auto ts_foreach_start = Timer::Now();

  if(testcase == "dash::for_each.g") {
    dash::for_each(begin, end, for_each);
  } else if(testcase == "dash::for_each_with_index.g") {
    dash::for_each_with_index(begin, end, for_each_index);
  } else if(testcase == "std::for_each.l") {
    std::for_each(lbegin, lend, for_each);
  }

  mes.time_foreach_s = Timer::ElapsedSince(ts_foreach_start) / (1000 * 1000);
  mes.time_total_s   = Timer::ElapsedSince(ts_tot_start) / (1000 * 1000);
  mes.local_elems_s  = lsize / mes.time_foreach_s;
  mes.local_size_mb  = static_cast<double>((lsize * sizeof(int))/(1024*1024));
  mes.testcase       = testcase;
  return mes;
}

void print_measurement_header()
{
  if (dash::myid() == 0) {
    cout << std::right
         << std::setw( 5) << "units"      << ","
         << std::setw( 9) << "mpi.impl"   << ","
         << std::setw(12) << "l.size.mb"  << ","
         << std::setw(13) << "l.elems/s"  << ","
         << std::setw(30) << "impl"       << ","
         << std::setw( 8) << "fill.s"     << ","
         << std::setw( 8) << "foreach.s"  << ","
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
         << std::fixed << setprecision(2) << setw(12) << mes.local_size_mb  << ","
         << std::fixed << setprecision(2) << setw(12) << (mes.local_elems_s / 1000) << "k,"
         << std::fixed << setprecision(2) << setw(30) << mes.testcase       << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_fill_s    << ","
         << std::fixed << setprecision(2) << setw(14) << mes.time_foreach_s << ","
         << std::fixed << setprecision(2) << setw(12) << mes.time_total_s
         << endl;
  }
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base      = 1000;
  params.max_time       = 20;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      params.size_base      = atoi(argv[i+1]);
    }
    if (flag == "-tmax") {
      params.size_base      = atoi(argv[i+1]);
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
  bench_cfg.print_param("-sb",    "initial matrix size", params.size_base);
  bench_cfg.print_param("-tmax",  "max time in s per iteration", params.max_time);
  bench_cfg.print_section_end();
}

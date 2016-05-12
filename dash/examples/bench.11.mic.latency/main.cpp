#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>

#include <libdash.h>

using namespace std;

typedef dash::util::Timer<
dash::util::TimeMeasure::Clock
> Timer;

typedef struct benchmark_params_t {
  int rep_base;
  int size_base;
  int num_iterations;
  int skip;
  int num_repeats;
  int min_repeats;
  int size_min;

} benchmark_params;

benchmark_params parse_args(int argc, char** argv)
{
  benchmark_params params;
  params.size_base      = 1;
  params.num_iterations = 10000;
  params.skip = 1000;
  params.rep_base       = params.size_base;
  params.num_repeats    = 7;
  params.min_repeats    = 1;
  params.size_min       = 1;

  if (argc > 2)
  {
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
  bench_cfg.print_section_end();
}

int main(int argc, char ** argv)
{

  Timer::Calibrate(0);
  auto   ts_start        = Timer::Now();

  dash::util::BenchmarkParams bench_params("bench.11.mic.latency");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);

  auto bench_cfg = bench_params.config();

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  //auto size = dash::size();

  dart_barrier(DART_TEAM_ALL);

  //auto & split_team = dash::Team::All().locality_split(DART_LOCALITY_SCOPE_MODULE, split_num_groups);

  dart_domain_locality_t *dom_node0;
  dart_domain_locality(DART_TEAM_ALL, ".0", &dom_node0);

  /* .NODE.MODULE.NUMA.UNIT */
  //node 0 should have at least two domains (i.e., 2 different modules)
  if (dom_node0->num_domains < 2) {
    if (myid == 0)
    {
      cout << "The benchmark must run on a node with at least two different modules (e.g., host and mic)" << endl;
    }

    dash::finalize();
    return EXIT_FAILURE;
  }

  dart_domain_locality_t * dom_node0_mod0;
  dart_domain_locality(DART_TEAM_ALL, ".0.0", &dom_node0_mod0);

  dart_domain_locality_t *dom_node0_mod1;
  dart_domain_locality(DART_TEAM_ALL, ".0.1", &dom_node0_mod1);

  int src = dom_node0_mod0->unit_ids[0];
  int dst = dom_node0_mod1->unit_ids[0];

  auto num_iterations = params.num_iterations;
  auto num_repeats = params.num_repeats;
  auto size_inc = params.size_min;
  auto skip = params.skip;

  char *src_mem = nullptr;

  if (myid == src) {
    size_t size_max = std::pow(params.size_base, params.num_repeats);
    src_mem = new char[size_max];
    memset(src_mem, 'a', size_max);
  }

  for (auto rep = 0; rep < num_repeats; ++rep)
  {
    auto mem_size = std::pow(params.size_base, rep) * size_inc;

    dash::GlobMem<char> *glob_mem = new dash::GlobMem<char>(mem_size, dash::Team::All());
    memset(glob_mem->lbegin(), 0x00, mem_size);

    if (myid == src) {
      //for (auto iter = 0; iter < num_iterations + skip; ++iter) {
      for (auto iter = 0; iter < 1; ++iter) {
        if (iter == skip) {
          ts_start = Timer::Now();
        }
        dart_put_blocking(glob_mem->at(dst, 0), src_mem, mem_size);
      }
      auto ts_end = Timer::Now();
      cout << "Latency with message size " << mem_size << "is: " << (ts_end - ts_start) * 1.0e6 / num_iterations;
    }

    dart_barrier(DART_TEAM_ALL);

    if (myid == dst) {
      for (auto idx = 0; idx < mem_size; ++idx) {
        DASH_ASSERT_EQ(glob_mem->lbegin()[idx], 'a', "invalid value");
      }
    }

    dart_barrier(DART_TEAM_ALL);

    delete glob_mem;
  }

  delete[] src_mem;
  dash::finalize();

  return EXIT_SUCCESS;
}


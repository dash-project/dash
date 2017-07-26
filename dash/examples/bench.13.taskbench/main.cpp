/**
 * Measures the overhead of different tasking primitives:
 * create, yield, dependency handling.
 */

#include <iostream>
#include <stdlib.h>
#include <libdash.h>
#include <dash/dart/if/dart.h>

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;


typedef struct benchmark_params_t {
  size_t num_create_tasks;
  size_t num_yield_tasks;
} benchmark_params;

static
benchmark_params parse_args(int argc, char * argv[]);

static void
empty_task(void *data) {
  // nothing to do here
  (void)data;
}

static void
yielding_task(void *data) {
  size_t num_yields = *static_cast<size_t*>(data);
  for (size_t i = 0; i < num_yields; ++i) {
    dart_task_yield(-1);
  }
}

template<bool PrintOutput>
void
benchmark_task_creation(size_t num_tasks)
{
  Timer t;
  for (size_t i = 0; i < num_tasks; ++i) {
    dart_task_create(&empty_task, NULL, 0, NULL, 0);
  }
  dart_task_complete();
  dash::barrier();
  if (PrintOutput && dash::myid() == 0) {
    std::cout << "avg task creation/execution: " << t.Elapsed() / num_tasks / 2
              << "us" << std::endl;
  }
}

void
benchmark_task_remotedep_creation(size_t num_tasks)
{
  Timer t;
  dash::Array<int> array(dash::size());
  size_t target = (dash::myid() + 1) % dash::size();
  dart_task_dep_t dep;
  dep.gptr = array[dash::myid().id].dart_gptr();
  dep.type = DART_DEP_OUT;
  dep.epoch = 0;
  // create one output task
  dart_task_create(&empty_task, NULL, 0, &dep, 1);
  dep.gptr = array[target].dart_gptr();
  dep.type = DART_DEP_IN;
  for (size_t i = 1; i <= num_tasks; ++i) {
    dep.epoch = 0;
    dart_task_create(&empty_task, NULL, 0, &dep, 1);
  }
  dart_task_complete();
  dash::barrier();
  if (dash::myid() == 0) {
    std::cout << "avg task creation/execution with dependencies: "
              << t.Elapsed() / num_tasks / 2
              << "us" << std::endl;
  }
}

void
benchmark_task_yield(size_t num_yields)
{
  dart_task_create(&yielding_task, &num_yields, sizeof(num_yields), NULL, 0);
  dart_task_create(&yielding_task, &num_yields, sizeof(num_yields), NULL, 0);
  Timer t;
  dart_task_complete();
  dash::barrier();
  if (dash::myid() == 0) {
    std::cout << "avg task creation/execution: " << t.Elapsed() / num_yields
              << "us" << std::endl;
  }
}

int main(int argc, char** argv)
{
  dash::init(&argc, &argv);
  if (!dash::is_multithreaded()) {
    std::cout << "Support for multithreading missing in DASH. Aborting!"
              << std::endl;
    dash::finalize();
    exit(-1);
  }

  dash::util::BenchmarkParams bench_params("bench.13.taskbench");
  bench_params.print_header();
  bench_params.print_pinning();

  benchmark_params params = parse_args(argc, argv);
  auto bench_cfg = bench_params.config();


  Timer::Calibrate();
  // warm up
  benchmark_task_creation<false>(params.num_create_tasks);

  benchmark_task_creation<true>(params.num_create_tasks);
  benchmark_task_yield(params.num_yield_tasks);

  if (dash::size() > 1) {
    benchmark_task_remotedep_creation(params.num_create_tasks);
  }

  dash::finalize();
}

static
benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.num_create_tasks = 100000;
  params.num_yield_tasks  = 1000000;

  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-t" || flag == "--num-tasks") {
      params.num_create_tasks = atoll(argv[i+1]);
    }
    if (flag == "-y" || flag == "--num-yields") {
      params.num_yield_tasks  = atoll(argv[i+1]);
    }
  }
  return params;
}


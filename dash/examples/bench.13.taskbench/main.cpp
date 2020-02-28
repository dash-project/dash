/**
 * Measures the overhead of different tasking primitives:
 * create, yield, dependency handling.
 */

#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <libdash.h>
#include <dash/dart/if/dart.h>
#include <dash/tasks/Tasks.h>

#ifdef DASH_EXAMPLES_TASKSUPPORT

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

struct hwc_cnt {
  long long int ins = 0; // instructions
  long long int cyc = 0; // cycles
};

static bool hwc_avail = true;

#ifdef DASH_ENABLE_PAPI
#include <papi.h>

static int EventSet=PAPI_NULL;

static void hwc_init()
{
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT && retval > 0) {
    throw ::std::runtime_error("PAPI version mismatch");
  }
  else if (retval < 0) {
    throw ::std::runtime_error("PAPI init failed");
  }
  PAPI_create_eventset(&EventSet);
  if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK) {
    std::cout << "Could not add PAPI_TOT_INS to event set!" << std::endl;
    hwc_avail = false;
  }
  if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK) {
    std::cout << "Could not add PAPI_TOT_INS to event set!" << std::endl;
    hwc_avail = false;
  }
  if (PAPI_start(EventSet) != PAPI_OK) {
    std::cout << "Could not start event set!" << std::endl;
    hwc_avail = false;
  }
}

static void hwc_fini()
{
  long_long values[2];
  PAPI_stop(EventSet, values);
}

// return the current instruction counter value
static hwc_cnt hwc_ins()
{
  long_long vals[2];
  PAPI_read(EventSet, vals);

  hwc_cnt ret = { vals[0], vals[1] };

  return ret;
}

#else

static void hwc_init()
{
  std::cout << "hwc_init: no hardware counters available!" << std::endl;
}

static void hwc_fini()
{ }

static hwc_cnt hwc_ins()
{
  return hwc_cnt{};
}

#endif // DASH_ENABLE_PAPI


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
  size_t n = 0;
  size_t num_yields = *static_cast<size_t*>(data);
  for (size_t i = 0; i < num_yields; ++i) {
    dart_task_yield(-1);
    ++n;
  }
  assert(n == num_yields);
}

template<bool PrintOutput>
void
benchmark_task_creation(size_t num_tasks)
{
  Timer t;
  auto start_hwc = hwc_ins();
  for (size_t i = 0; i < num_tasks; ++i) {
    dash::tasks::async([](){});
  }
  dash::tasks::complete();
  auto end_hwc = hwc_ins();
  auto elapsed = t.Elapsed();
  dash::barrier();
  if (PrintOutput && dash::myid() == 0) {
    std::cout << "avg task creation/execution: " << elapsed / num_tasks << "us : ";
    if (hwc_avail) {
      std::cout << (end_hwc.ins - start_hwc.ins) / num_tasks << " ins : "
                << (end_hwc.cyc - start_hwc.cyc) / num_tasks << " cyc : ";
    }
    std::cout << std::endl;
  }
}

template<bool PrintOutput>
void
benchmark_tasklet_creation(size_t num_tasks)
{
  Timer t;
  auto start_hwc = hwc_ins();
  for (size_t i = 0; i < num_tasks; ++i) {
    dash::tasks::tasklet([](){});
  }
  dash::tasks::complete();
  auto end_hwc = hwc_ins();
  auto elapsed = t.Elapsed();
  dash::barrier();
  if (PrintOutput && dash::myid() == 0) {
    std::cout << "avg tasklet creation/execution: " << elapsed / num_tasks << "us : ";
    if (hwc_avail) {
      std::cout << (end_hwc.ins - start_hwc.ins) / num_tasks << " ins : "
                << (end_hwc.cyc - start_hwc.cyc) / num_tasks << " cyc : ";
    }
    std::cout << std::endl;
  }
}


#if 0
void
benchmark_task_remotedep_creation(size_t num_tasks)
{
  Timer t;
  dash::Array<int> array(dash::size());
  size_t target = (dash::myid() + 1) % dash::size();
  dart_task_dep_t dep;
  dep.gptr = array[dash::myid().id].dart_gptr();
  dep.type = DART_DEP_OUT;
  dep.phase = DART_PHASE_ANY;
  // create one output task
  dart_task_create(&empty_task, NULL, 0, &dep, 1, DART_PRIO_LOW);
  dep.gptr = array[target].dart_gptr();
  dep.type = DART_DEP_IN;
    dep.phase = DART_PHASE_ANY;
  for (size_t i = 1; i <= num_tasks; ++i) {
    dart_task_create(&empty_task, NULL, 0, &dep, 1, DART_PRIO_LOW);
  }
  dart_task_complete();
  dash::barrier();
  if (dash::myid() == 0) {
    std::cout << "avg task creation/execution with dependencies: "
              << t.Elapsed() / num_tasks / 2
              << "us" << std::endl;
  }
}
#endif

template<bool RootOnly, bool UseInDep=true>
void
benchmark_task_remotedep_creation(size_t num_tasks, int num_deps)
{
  dash::Array<double> array(dash::size()*num_deps);
  int target = (dash::myid() + 1) % dash::size();

  Timer t;

  dash::tasks::async_fence();

  if (!RootOnly || dash::myid() == 0) {
    dash::tasks::taskletloop(0UL, num_tasks, dash::tasks::chunk_size(1),
      [](int from, int to){
        // nothing to do
        //std::cout << "task " << from << std::endl;
      },
      [=, &array](
        auto from,
        auto to,
        auto inserter) {
        for (int d = 0; d < num_deps; d++) {
          if (UseInDep) {
            *inserter = dash::tasks::in(array[target*num_deps + d]);
          } else {
            *inserter = dash::tasks::out(array[target*num_deps + d]);
          }
        }
      }
    );
  }

  dash::tasks::complete();
  if (dash::myid() == 0) {
    std::cout << "remotedeps:" << num_deps
              << ":" << (RootOnly ? "root" : "all") << ":"
              << t.Elapsed() / num_tasks
              << "us" << std::endl;
  }
}

template<bool RootOnly>
void
benchmark_task_spreadremotedep_creation(size_t num_tasks, int num_deps)
{
  dash::Array<double> array(dash::size()*num_deps);
  int target = (dash::myid() + 1) % dash::size();

  Timer t;

  dash::tasks::async_fence();

  if (!RootOnly || dash::myid() == 0) {
    dash::tasks::taskletloop(0UL, num_tasks, dash::tasks::chunk_size(1),
      [](int from, int to){
        // nothing to do
        //std::cout << "task " << from << std::endl;
      },
      [=, &array](
        auto from,
        auto to,
        auto inserter) {
        int t = (dash::myid() + 1) % dash::size();
        for (int d = 0; d < num_deps; d++) {
          *inserter = dash::tasks::in(array[target*num_deps + d]);

          t = (t+1) % dash::size();
          if (t == dash::myid()) t = (t+1) % dash::size();
        }
      }
    );
  }

  dash::tasks::complete();
  if (dash::myid() == 0) {
    std::cout << "spreadremotedeps:" << num_deps
              << ":" << (RootOnly ? "root" : "all") << ":"
              << t.Elapsed() / num_tasks
              << "us" << std::endl;
  }
}


void
benchmark_task_localdep_creation(size_t num_tasks, int num_deps)
{
  dash::Array<double> array(dash::size()*num_deps);
  int target = (dash::myid() + 1) % dash::size();

  double *tmp = new double[num_deps];

  Timer t;
  //for (size_t i = 1; i <= num_tasks; ++i) {
    dash::tasks::taskletloop(0UL, num_tasks, dash::tasks::chunk_size(1),
      [](int from, int to){
        // nothing to do
        //std::cout << "task " << from << std::endl;
      },
      [=, &array](
        auto from,
        auto to,
        auto inserter) {
        for (int d = 0; d < num_deps; d++) {
          *inserter = dash::tasks::out(tmp[d]);
        }
      }
    );
  //}

  dash::tasks::complete();
  if (dash::myid() == 0) {
    std::cout << "localdeps:" << num_deps
              << ":"
              << t.Elapsed() / num_tasks
              << "us" << std::endl;
  }
  delete[] tmp;
}


void
benchmark_task_yield(size_t num_yields)
{
  dart_task_create(&yielding_task, &num_yields, sizeof(num_yields), NULL, 0, DART_PRIO_LOW, 0, NULL);
  dart_task_create(&yielding_task, &num_yields, sizeof(num_yields), NULL, 0, DART_PRIO_LOW, 0, NULL);
  Timer t;
  auto start_hwc = hwc_ins();
  dart_task_complete(true);
  auto end_hwc = hwc_ins();
  if (dash::myid() == 0) {
    std::cout << "avg task yield: " << t.Elapsed() / num_yields / 2 << "us : ";
    if (hwc_avail) {
      std::cout << (end_hwc.ins - start_hwc.ins) / num_yields / 2 << " ins : "
                << (end_hwc.cyc - start_hwc.cyc) / num_yields / 2 << " cyc : ";
    }
    std::cout << std::endl;
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

  if (dash::tasks::numthreads() > 1) {
    std::cout << "WARN: Found more than one (" << dash::tasks::numthreads()
              << ") threads, results may be not be accurate!"
              << std::endl;
  }

  hwc_init();

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

  benchmark_tasklet_creation<true>(params.num_create_tasks);

  for (int i = 1; i <= 32; i*=2) {
    benchmark_task_localdep_creation(params.num_create_tasks, i);
  }

  if (dash::size() > 1) {
    for (int i = 1; i <= 32; i*=2) {
      benchmark_task_spreadremotedep_creation<true>(params.num_create_tasks, i);
    }
  }


  if (dash::size() > 1) {
    for (int i = 1; i <= 32; i*=2) {
      benchmark_task_spreadremotedep_creation<false>(params.num_create_tasks, i);
    }
  }

  if (dash::size() > 1) {
    for (int i = 1; i <= 32; i*=2) {
      benchmark_task_remotedep_creation<true>(params.num_create_tasks, i);
    }
  }

  if (dash::size() > 1) {
    for (int i = 1; i <= 32; i*=2) {
      benchmark_task_remotedep_creation<false>(params.num_create_tasks, i);
    }
  }

  hwc_fini();

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

#else

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  std::cout << "Skipping example due to missing task support" << std::endl;

  dash::finalize();

  return 0;
}

#endif // DASH_EXAMPLES_TASKSUPPORT

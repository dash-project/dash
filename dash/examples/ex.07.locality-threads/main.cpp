#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>

#include <libdash.h>

using namespace std;
using namespace dash;

typedef struct unit_threading_s {
  int  num_threads;
  int  max_threads;
  bool hyperthreads;
  bool openmp;
} unit_threading_t;

unit_threading_t get_local_threading();
int multithread_task(int n_threads);


int main(int argc, char * argv[])
{
  // Note: barriers and sleeps are only required to prevent output of
  //       different units to interleave.

  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);

  dash::util::BenchmarkParams bench_params("ex.07.locality-threads");
  bench_params.print_header();
  bench_params.print_pinning();

  dart_barrier(DART_TEAM_ALL);
  sleep(3);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<unit_threading_t> unit_threading(size);
  dash::Array<int>              unit_omp_threads(size);

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  dart_barrier(DART_TEAM_ALL);
  sleep(1);

  // To prevent interleaving output:
  std::ostringstream i_os;
  i_os << "Process started at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << i_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (myid == 0) {
    cout << separator << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_team_locality(
      DART_TEAM_ALL, ".", &global_domain_locality);

    cout << *global_domain_locality
         << endl
         << separator
         << endl;
  } else {
    sleep(2);
  }
  dart_barrier(DART_TEAM_ALL);

  // ========================================================================
  // Collect the units' threading settings:
  // ========================================================================
  unit_threading.local[0] = get_local_threading();
  dart_barrier(DART_TEAM_ALL);

  // ========================================================================
  // Print the units' threading settings:
  // ========================================================================
  if (myid == 0) {
    int u = 0;
    for (auto it = unit_threading.begin(); it != unit_threading.end(); ++it) {
      unit_threading_t ut = *it;
      std::cout << "unit "          << std::setw(3) << u << ": "
                << "num_threads: "  << std::setw(3) << ut.num_threads  << ", "
                << "max_threads: "  << std::setw(3) << ut.max_threads  << ", "
                << "hyperthreads: " << std::setw(3) << ut.hyperthreads << ", "
                << "openmp: "       << ut.openmp
                << std::endl;
      u++;
    }
  }
  dart_barrier(DART_TEAM_ALL);

  // ========================================================================
  // Run a multi-threaded task:
  // ========================================================================
  int n_omp_threads = multithread_task(unit_threading.local[0].num_threads);
  unit_omp_threads.local[0] = n_omp_threads;
  dart_barrier(DART_TEAM_ALL);

  // ========================================================================
  // Print the units' number of OMP threads used:
  // ========================================================================
  if (myid == 0) {
    int u = 0;
    for (auto it = unit_omp_threads.begin(); it != unit_omp_threads.end();
         ++it) {
      int omp_threads = *it;
      std::cout << "unit "          << std::setw(3) << u << ": "
                << "OMP threads: "  << std::setw(3) << omp_threads
                << std::endl;
      u++;
    }
  } else {
    sleep(2);
  }
  dart_barrier(DART_TEAM_ALL);


  std::ostringstream f_os;
  f_os << "Process exiting at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << f_os.str();

  dart_barrier(DART_TEAM_ALL);
  dash::finalize();

  return EXIT_SUCCESS;
}

unit_threading_t get_local_threading()
{
  unit_threading_t ut;
  ut.num_threads  = dash::util::Locality::NumCores();
  ut.max_threads  = dash::util::Locality::MaxThreads();
  ut.hyperthreads = false;
#ifdef DASH_ENABLE_OPENMP
  ut.openmp       = true;
#else
  ut.openmp       = false;
#endif

  if (dash::util::Config::get<bool>("DASH_DISABLE_THREADS")) {
    // Threads disabled in unit scope:
    ut.num_threads  = 1;
  } else if (dash::util::Config::get<bool>("DASH_MAX_SMT")) {
    // Configured to use SMT (hyperthreads):
    ut.num_threads *= dash::util::Locality::MaxThreads();
    ut.hyperthreads = true;
  } else {
    // Start one thread on every physical core assigned to this unit:
    ut.num_threads *= dash::util::Locality::MinThreads();
  }
  if (dash::util::Config::is_set("DASH_MAX_UNIT_THREADS")) {
    ut.max_threads  = dash::util::Config::get<int>(
                        "DASH_MAX_UNIT_THREADS");
    ut.num_threads  = std::min(ut.max_threads, ut.num_threads);
  }
  return ut;
}

int multithread_task(int n_threads)
{
  int n_omp_threads = 0;
#ifdef DASH_ENABLE_OPENMP
  std::ostringstream ss;
  ss << "omp task [ "
     << "unit "   << std::setw(3) << dash::myid() << " | thread ids: ";
  if (n_threads > 1) {
    int t_id;
    #pragma omp parallel num_threads(n_threads) \
                         private(t_id) \
                         shared(n_omp_threads, ss)
    {
      t_id = omp_get_thread_num();
      #pragma omp critical
      {
        ss << t_id << " ";
        n_omp_threads++;
      }
    }
  }
  ss << "]";
  std::cout << ss.str() << std::endl;
#endif
  return n_omp_threads;
}

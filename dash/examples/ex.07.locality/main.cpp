#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>

#include <libdash.h>

using namespace std;
using namespace dash;


int main(int argc, char * argv[])
{
  float fsleep = 1;
  if (argc > 1 && std::string(argv[1]) == "-nw") {
    fsleep = 0;
  }

  // Note: barriers and sleeps are only required to prevent output of
  //       different units to interleave.

  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);

  dash::util::BenchmarkParams bench_params("ex.07.locality");
  bench_params.print_header();
  bench_params.print_pinning();

  dart_barrier(DART_TEAM_ALL);
  sleep(3 * fsleep);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  dart_barrier(DART_TEAM_ALL);
  sleep(1 * fsleep);

  // To prevent interleaving output:
  std::ostringstream i_os;
  i_os << "Process started at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << i_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(2 * fsleep);

  if (myid == 0) {
    cout << separator << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_team_locality(
      DART_TEAM_ALL, ".", &global_domain_locality);

    cout << "Hint: run using numactl, for example: "
         << endl
         << "  numactl --physcpubind=6,7,8,9,10,11,12,13,14,15,16,17 \\"
         << endl
         << "     mpirun -n 12 ./bin/ex.07.locality.mpi"
         << endl
         << endl;

    cout << ((dash::util::LocalityJSONPrinter()
              << *global_domain_locality)).str()
         << endl
         << separator
         << endl;
  } else {
    sleep(2 * fsleep);
  }

  // To prevent interleaving output:
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

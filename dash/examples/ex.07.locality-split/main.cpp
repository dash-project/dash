#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
#include <sstream>

#include <libdash.h>

#include <dash/util/LocalityJSONPrinter.h>


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
  bool locality_split   = false;
  int  split_num_groups = 3;
  dart_locality_scope_t split_scope = DART_LOCALITY_SCOPE_NODE;

  if (argc >= 2) {
    if (std::string(argv[1]) == "-s") {
      locality_split   = false;
      split_num_groups = static_cast<int>(strtol(argv[2], NULL, 10));
    } else if (std::string(argv[1]) == "-ls") {
      locality_split   = true;
      split_scope      = DART_LOCALITY_SCOPE_NODE;
      if (argc >= 3) {
          if (std::string(argv[2]) == "node") {
            split_scope = DART_LOCALITY_SCOPE_NODE;
          } else if (std::string(argv[2]) == "module") {
            split_scope = DART_LOCALITY_SCOPE_MODULE;
          } else if (std::string(argv[2]) == "numa") {
            split_scope = DART_LOCALITY_SCOPE_NUMA;
          } else if (std::string(argv[2]) == "core") {
            split_scope = DART_LOCALITY_SCOPE_CORE;
          }
          if (argc >= 4) {
            split_num_groups = static_cast<int>(strtol(argv[3], NULL, 10));
          }
      }
    }
  }

  dash::init(&argc, &argv);

  dash::util::BenchmarkParams bench_params("ex.07.locality-split");
  bench_params.print_header();
  bench_params.print_pinning();

  dart_barrier(DART_TEAM_ALL);
  sleep(5 * fsleep);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  dart_barrier(DART_TEAM_ALL);
  sleep(2 * fsleep);
  if (myid == 0) {
    cout << "Usage:" << endl
         << "  ex.07.locality [-s <num_split_groups> | -ls <split_scope>]"
         << endl
         << endl
         << "  ex.07.locality ";
    if (locality_split) {
      cout << "-ls " << split_scope << " " << split_num_groups << ": "
           << "locality split into " << split_num_groups << " groups "
           << "at scope " << split_scope << endl;
    } else {
      cout << "-s " << split_num_groups << ": "
           << "regular split into " << split_num_groups << " groups"
           << endl;
    }
    cout << separator << endl;
  } else {
    sleep(2 * fsleep);
  }
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
  sleep(5 * fsleep);

  if (myid == 0) {
    cout << separator << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_team_locality(
      DART_TEAM_ALL, ".", &global_domain_locality);

    cout << ((dash::util::LocalityJSONPrinter()
              << *global_domain_locality)).str()
         << endl
         << separator
         << endl;
  } else {
    sleep(2 * fsleep);
  }

  auto & split_team = locality_split
                      ? dash::Team::All().locality_split(split_scope,
                                                         split_num_groups)
                      : dash::Team::All().split(split_num_groups);

  std::ostringstream t_os;
  t_os << "Unit id " << setw(3) << myid << " -> "
       << "unit id " << setw(3) << split_team.myid() << " "
       << "in team " << split_team.dart_id() << " after split"
       << endl;
  cout << t_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(2 * fsleep);

  for (int g = 0; g < split_num_groups; ++g) {
    if (split_team.dart_id() == 1+g && split_team.myid() == 0) {
      cout << "Locality domains of unit 0 "
           << "in team " << split_team.dart_id() << ":" << endl
           << endl;

      dart_domain_locality_t * global_domain_locality;
      dart_domain_team_locality(
        split_team.dart_id(), ".", &global_domain_locality);

      cout << ((dash::util::LocalityJSONPrinter()
                << *global_domain_locality)).str()
           << endl
           << separator
           << endl;
    } else {
      sleep(2 * fsleep);
    }
    dart_barrier(DART_TEAM_ALL);
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

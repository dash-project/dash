#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

int main(int argc, char * argv[])
{
  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);

  dart_barrier(DART_TEAM_ALL);
  sleep(10);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  // To avoid interleaving output:
  std::ostringstream os;
  os << "Process started at "
     << "unit " << myid << " of " << size << " "
     << "on "   << buf  << " pid=" << pid
     << endl;

  cout << os.str();

  dart_unit_locality_t * uloc;
  dart_ret_t             ret = dart_unit_locality(myid, &uloc);
  if (ret != DART_OK) {
    cerr << "Error: dart_unit_locality(" << myid << ") failed"
         << endl;
    return EXIT_FAILURE;
  }

  dart_barrier(DART_TEAM_ALL);
  sleep(10);

  if (myid == 0) {
    std::ostringstream ls;
    for (unsigned u = 0; u < size; ++u) {
      dart_unit_locality(u, &uloc);
      ls << "unit " << u << " locality: " << endl
         << "  unit:        " << uloc->unit               << endl
         << "  host:        " << uloc->host               << endl
         << "  domain:      " << uloc->domain_tag         << endl
         << "  numa_id:     " << uloc->hwinfo.numa_id     << endl
         << "  core_id:     " << uloc->hwinfo.cpu_id      << endl
         << "  num_cores:   " << uloc->hwinfo.num_cores   << endl
         << "  min_cpu_mhz: " << uloc->hwinfo.min_cpu_mhz << endl
         << "  max_cpu_mhz: " << uloc->hwinfo.max_cpu_mhz << endl
         << "  min_threads: " << uloc->hwinfo.min_threads << endl
         << "  max_threads: " << uloc->hwinfo.max_threads << endl
         << endl;
      cout << ls.str();
    }
  } else {
    sleep(5);
  }

  dart_barrier(DART_TEAM_ALL);
  sleep(5);

  dash::finalize();

  return EXIT_SUCCESS;
}

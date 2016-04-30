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

#if 0
  dart_unit_locality_t * uloc;
  dart_ret_t             ret = dart_unit_locality(myid, &uloc);
  if (ret != DART_OK) {
    cerr << "Error: dart_unit_locality(" << myid << ") failed"
         << endl;
    return EXIT_FAILURE;
  }

  if (myid == 0) {
    std::ostringstream ls;
    for (int u = 0; u < size; ++u) {
      ls << "unit " << u << " locality: " << endl
         << "  unit:        " << uloc->unit        << endl
         << "  host:        " << uloc->host        << endl
         << "  domain:      " << uloc->domain_tag  << endl
         << "  numa_id:     " << uloc->numa_id     << endl
         << "  core_id:     " << uloc->core_id     << endl
         << "  num_cores:   " << uloc->num_cores   << endl
         << "  min_cpu_mhz: " << uloc->min_cpu_mhz << endl
         << "  max_cpu_mhz: " << uloc->max_cpu_mhz << endl
         << "  num_threads: " << uloc->num_threads << endl
         << endl;
      cout << ls.str();
    }
  }
#endif

  dash::finalize();

  return EXIT_SUCCESS;
}

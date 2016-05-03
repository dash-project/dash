#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

std::ostream & operator<<(
  std::ostream& os,
  dart_locality_scope_t e)
{
  switch(e) {
    case DART_LOCALITY_SCOPE_GLOBAL: os << "GLOBAL";    break;
    case DART_LOCALITY_SCOPE_NODE:   os << "NODE";      break;
    case DART_LOCALITY_SCOPE_MODULE: os << "MODULE";    break;
    case DART_LOCALITY_SCOPE_NUMA:   os << "NUMA";      break;
    case DART_LOCALITY_SCOPE_UNIT:   os << "UNIT";      break;
    case DART_LOCALITY_SCOPE_CORE:   os << "CORE";      break;
    default:                         os << "UNDEFINED"; break;
  }
  return os;
}

void print_domain(
  dart_domain_locality_t * domain)
{
  std::string indent(domain->level * 4, ' ');

  cout << indent << "level:  " << domain->level
       << endl
       << indent << "scope:  " << domain->scope
       << endl
       << indent << "domain: " << domain->domain_tag
       << endl;

  if (domain->level == 0) {
    cout << indent << "nodes:  " << domain->num_nodes << endl;
  } else {
    cout << indent << "host:   " << domain->host << endl;
  }
  if (domain->num_units > 0) {
    cout << indent << "- units: " << domain->num_units << endl;
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id = domain->unit_ids[u];
      dart_unit_locality_t * uloc;
      dart_unit_locality(unit_id, &uloc);
      cout << indent << "  units[" << setw(3) << u << "]: "
                     << setw(4) << unit_id << " "
                     << "unit locality:"
                     << endl;
      cout << indent << "  "
                     << "unit: "   << uloc->unit << " "
                     << "host: "   << uloc->host << " "
                     << "domain: " << uloc->domain_tag
                     << endl;
      cout << indent << "  "
                     << "hwinfo:"
                     << endl;
      cout << indent << "    "
                     << "numa_id: " << uloc->hwinfo.numa_id << " "
                     << "cpu_id:  " << uloc->hwinfo.cpu_id  << " "
                     << "threads: " << uloc->hwinfo.min_threads << "..."
                                    << uloc->hwinfo.max_threads << " "
                     << "cpu_mhz: " << uloc->hwinfo.min_cpu_mhz << "..."
                                    << uloc->hwinfo.max_cpu_mhz
                     << endl;
    }
  }
  if (domain->num_domains > 0) {
    cout << indent << "- domains: " << domain->num_domains << endl;
    for (int d = 0; d < domain->num_domains; ++d) {
      cout << indent << "  domains[" << d << "]: " << endl;
      print_domain(&domain->domains[d]);
    }
  }
}

int main(int argc, char * argv[])
{
  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);

  dart_barrier(DART_TEAM_ALL);
  sleep(5);

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
  sleep(5);

  if (myid == 0) {
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(".", &global_domain_locality);
    print_domain(global_domain_locality);
#if 0
    for (unsigned u = 0; u < size; ++u) {
      dart_unit_locality(u, &uloc);
      std::ostringstream ls;
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
#endif
  } else {
    sleep(5);
  }

  dart_barrier(DART_TEAM_ALL);
  sleep(5);

  dash::finalize();

  return EXIT_SUCCESS;
}

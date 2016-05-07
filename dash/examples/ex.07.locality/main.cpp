#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <sstream>

#include <libdash.h>

using namespace std;

std::ostream & operator<<(
  std::ostream           & os,
  dart_locality_scope_t    scope);

void print_domain(
  dart_team_t              team,
  dart_domain_locality_t * domain);

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

  // To prevent interleaving output:
  std::ostringstream i_os;
  i_os << "Process started at "
       << "unit " << setw(3) << myid << " of "  << size << " "
       << "on "   << buf     << " pid:" << pid
       << endl;
  cout << i_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(5);

  if (myid == 0) {
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(DART_TEAM_ALL, ".", &global_domain_locality);
    print_domain(DART_TEAM_ALL, global_domain_locality);
  } else {
    sleep(5);
  }

  auto & split_team = dash::Team::All().split(3);

  std::ostringstream t_os;
  t_os << "Unit id " << setw(3) << myid << " -> "
       << "unit id " << setw(3) << split_team.myid() << " "
       << "in team " << split_team.dart_id() << " after split"
       << endl;
  cout << t_os.str();

  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (split_team.dart_id() == 1 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 1:" << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
  } else {
    sleep(5);
  }
  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (split_team.dart_id() == 2 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 2:" << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
  } else {
    sleep(5);
  }
  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (split_team.dart_id() == 3 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 3:" << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
  } else {
    sleep(5);
  }
  dart_barrier(DART_TEAM_ALL);
  sleep(2);

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

std::ostream & operator<<(
  std::ostream          & os,
  dart_locality_scope_t   scope)
{
  switch(scope) {
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
  dart_team_t              team,
  dart_domain_locality_t * domain)
{
  const int max_level = 3;

  std::string indent;
  for (int i = 0; i < domain->level; ++i) {
    indent += ":   ";
  }

  cout << indent << "scope:   " << domain->scope << " "
                 << "(level "  << domain->level << ")"
       << endl
       << indent << "domain:  " << domain->domain_tag
       << endl;

  if (domain->level > max_level) {
    return;
  }

  if (domain->level == 0) {
    cout << indent << "nodes:   " << domain->num_nodes << endl;
  } else {
    cout << indent << "host:    " << domain->host << endl;
  }
  cout << indent << "units:   " << domain->num_units << endl;
  if (domain->level == 3) {
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      cout << indent << "|-- units[" << setw(3) << u << "]: " << unit_id
                     << endl;
      cout << indent << "|               unit:   "
                                         << uloc->unit
                                         << " in team "  << uloc->team
                                         << ", global: " << unit_gid
                     << endl;
      cout << indent << "|               domain: " << uloc->domain_tag
                     << endl;
      cout << indent << "|               host:   " << uloc->host
                     << endl;
      cout << indent << "|               hwinfo: "
                           << "numa_id: "
                              << uloc->hwinfo.numa_id << " "
                           << "cpu_id: "
                              << setw(3) << uloc->hwinfo.cpu_id  << " "
                           << "threads: "
                              << uloc->hwinfo.min_threads << "..."
                              << uloc->hwinfo.max_threads << " "
                           << "cpu_mhz: "
                              << uloc->hwinfo.min_cpu_mhz << "..."
                              << uloc->hwinfo.max_cpu_mhz
                           << endl;
    }
    if (domain->num_units > 0) {
      cout << indent << "'-----------"
           << endl;
    }
  }
  if (domain->level < max_level && domain->num_domains > 0) {
    cout << indent << "domains: " << domain->num_domains << endl;
    for (int d = 0; d < domain->num_domains; ++d) {
      cout << indent << "|-- domains[" << setw(3) << d << "]: " << endl;

      print_domain(team, &domain->domains[d]);

      cout << indent << "'----------"
           << endl
           << indent << endl;
    }
  }
}

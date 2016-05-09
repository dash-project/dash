#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <cstdlib>
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
      if (std::string(argv[2]) == "module") {
        split_scope = DART_LOCALITY_SCOPE_MODULE;
      } else if (std::string(argv[2]) == "numa") {
        split_scope = DART_LOCALITY_SCOPE_NUMA;
      }
      if (argc >= 4) {
        split_num_groups = static_cast<int>(strtol(argv[3], NULL, 10));
      }
    }
  }

  dash::init(&argc, &argv);

  dart_barrier(DART_TEAM_ALL);
  sleep(5);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  std::string separator(80, '=');

  dart_barrier(DART_TEAM_ALL);
  if (myid == 0) {
    cout << "Usage:" << endl
         << "  ex.07.locality [-s <num_split_groups> | -ls <split_scope>]"
         << endl
         << endl
         << "  ex.07.locality ";
    if (locality_split) {
      cout << "-ls " << argv[2] << " " << split_num_groups << ": "
           << "locality split into " << split_num_groups << " groups "
           << "at scope " << split_scope << endl;
    } else {
      cout << "-s " << split_num_groups << ": "
           << "regular split into " << split_num_groups << " groups" << endl;
    }
    cout << separator << endl;
  } else {
    sleep(1);
  }
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
  sleep(5);

  if (myid == 0) {
    cout << separator << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(DART_TEAM_ALL, ".", &global_domain_locality);
    print_domain(DART_TEAM_ALL, global_domain_locality);
    cout << separator << endl;
  } else {
    sleep(5);
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
  sleep(2);

  if (split_team.dart_id() == 1 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 1:" << endl
         << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
    cout << separator << endl;
  } else {
    sleep(5);
  }
  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (split_team.dart_id() == 2 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 2:" << endl
         << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
    cout << separator << endl;
  } else {
    sleep(5);
  }
  dart_barrier(DART_TEAM_ALL);
  sleep(2);

  if (split_team.dart_id() == 3 && split_team.myid() == 0) {
    cout << "Locality domains of unit 0 in team 3:" << endl
         << endl;
    dart_domain_locality_t * global_domain_locality;
    dart_domain_locality(split_team.dart_id(), ".", &global_domain_locality);
    print_domain(split_team.dart_id(), global_domain_locality);
    cout << separator << endl;
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

  std::string indent(domain->level * 4, ' ');

  cout << indent << "scope:   " << domain->scope << " "
                 << "(level "  << domain->level << ")"
       << endl
       << indent << "domain:  " << domain->domain_tag
       << endl;

  if (domain->level > max_level) {
    return;
  }

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    cout << indent << "nodes:   " << domain->num_nodes << endl;
  } else {
    cout << indent << "host:    " << domain->host            << endl
         << indent << "NUMAs:   " << domain->hwinfo.num_numa << endl;
  }
  cout << indent << "units:   " << domain->num_units << ": { ";
  for (int u = 0; u < domain->num_units; ++u) {
    cout << domain->unit_ids[u];
    if (u < domain->num_units-1) {
      cout << ", ";
    }
  }
  cout << " }" << endl;

  if (domain->level == 3) {
    std::string uindent((domain->level + 1) * 4, ' ');
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      cout << uindent << "|-- units[" << setw(2) << u << "]: " << unit_id
                      << endl;
      cout << uindent << "|              unit:   "
                                         << uloc->unit
                                         << " in team "  << uloc->team
                                         << ", global: " << unit_gid
                      << endl;
      cout << uindent << "|              domain: " << uloc->domain_tag
                      << endl;
      cout << uindent << "|              host:   " << uloc->host
                      << endl;
      cout << uindent << "|              hwinfo: "
                            << "numa_id: "
                               << uloc->hwinfo.numa_id << " "
                            << "cpu_id: "
                               << uloc->hwinfo.cpu_id  << " "
                            << "threads: "
                               << uloc->hwinfo.min_threads << "..."
                               << uloc->hwinfo.max_threads << " "
                            << "cpu_mhz: "
                               << uloc->hwinfo.min_cpu_mhz << "..."
                               << uloc->hwinfo.max_cpu_mhz
                            << endl;
    }
  }
  if (domain->level < max_level && domain->num_domains > 0) {
    cout << indent << "domains: " << domain->num_domains << endl;
    for (int d = 0; d < domain->num_domains; ++d) {
      cout << indent << "|-- domains[" << setw(2) << d << "]: " << endl;

      print_domain(team, &domain->domains[d]);

      cout << indent << "'----------"
           << endl;
    }
  }
}

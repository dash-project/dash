#ifndef DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED
#define DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED

#include <sstream>
#include <iostream>
#include <iomanip>

#include <libdash.h>

std::ostream & operator<<(
  std::ostream          & os,
  dart_locality_scope_t   scope)
{
  switch(scope) {
    case DART_LOCALITY_SCOPE_GLOBAL:  os << "GLOBAL";    break;
    case DART_LOCALITY_SCOPE_GROUP:   os << "GROUP";     break;
    case DART_LOCALITY_SCOPE_NETWORK: os << "NETWORK";   break;
    case DART_LOCALITY_SCOPE_NODE:    os << "NODE";      break;
    case DART_LOCALITY_SCOPE_MODULE:  os << "MODULE";    break;
    case DART_LOCALITY_SCOPE_NUMA:    os << "NUMA";      break;
    case DART_LOCALITY_SCOPE_UNIT:    os << "UNIT";      break;
    case DART_LOCALITY_SCOPE_CORE:    os << "CORE";      break;
    default:                          os << "UNDEFINED"; break;
  }
  return os;
}

void print_domain(
  dart_team_t              team,
  dart_domain_locality_t * domain)
{
  using namespace std;

  const int max_level = 5;

  std::string indent(domain->level * 4, ' ');

  cout << indent << "scope:   " << domain->scope << " "
                 << "(level "   << domain->level << ")"
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
    cout << indent << "NUMAs:   " << domain->hwinfo.num_numa << endl;
  }
  cout << indent << "units:   " << domain->num_units << ": global ids { ";
  for (int u = 0; u < domain->num_units; ++u) {
    dart_unit_t g_unit_id;
    dart_team_unit_l2g(domain->team, domain->unit_ids[u], &g_unit_id);
    cout << g_unit_id;
    if (u < domain->num_units-1) {
      cout << ", ";
    }
  }
  cout << " }" << endl;

#if 0
  if (domain->scope == DART_LOCALITY_SCOPE_NUMA ||
      domain->scope == DART_LOCALITY_SCOPE_GROUP) {
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
#endif
  if (domain->level < max_level && domain->num_domains > 0) {
    cout << indent << "domains: " << domain->num_domains << endl;
    for (int d = 0; d < domain->num_domains; ++d) {
      if (static_cast<int>(domain->domains[d].scope) <=
          static_cast<int>(DART_LOCALITY_SCOPE_CORE)) {
        cout << indent << "|-- domains[" << setw(2) << d << "]: " << endl;

        print_domain(team, &domain->domains[d]);

        cout << indent << "'----------"
             << endl;
      }
    }
  }
}

#endif // DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED

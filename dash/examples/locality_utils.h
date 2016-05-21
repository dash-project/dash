#ifndef DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED
#define DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED

#include <sstream>
#include <iostream>
#include <iomanip>

#include <libdash.h>


void print_domain(
  dart_team_t              team,
  dart_domain_locality_t * domain,
  std::string              indent = "")
{
  using namespace std;

  cout << indent << "scope:   " << domain->scope << " "
                                << "(level " << domain->level << ")"
       << endl
       << indent << "rel.idx: " << domain->relative_index
       << endl;

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    cout << indent << "nodes:   " << domain->num_nodes << endl;
  }
  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NUMA)) {
    cout << indent << "NUMAs:   " << domain->hwinfo.num_numa << endl;
  } else {
    cout << indent << "NUMA id: " << domain->hwinfo.numa_id  << endl;
  }
  cout << indent << "units:   " << "{ ";
  for (int u = 0; u < domain->num_units; ++u) {
    dart_unit_t g_unit_id;
    dart_team_unit_l2g(domain->team, domain->unit_ids[u], &g_unit_id);
    cout << g_unit_id;
    if (u < domain->num_units-1) {
      cout << ", ";
    }
  }
  cout << " }" << endl;

  if (domain->scope == DART_LOCALITY_SCOPE_CORE) {
    std::string uindent = indent;
    uindent += std::string(9, ' ');

    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      cout << uindent << "unit id:   " << uloc->unit << "  ("
                                       << "in team " << uloc->team << ", "
                                       << "global: " << unit_gid   << ")"
                      << endl;
      cout << uindent << "domain:    " << uloc->domain_tag
                      << endl;
      cout << uindent << "host:      " << uloc->host
                      << endl;
      cout << uindent << "hwinfo:    " << "numa_id: "
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

  if (domain->num_domains > 0) {
    cout << indent << "domains: " << domain->num_domains << endl;

    for (int d = 0; d < domain->num_domains; ++d) {
      if (static_cast<int>(domain->domains[d].scope) <=
          static_cast<int>(DART_LOCALITY_SCOPE_CORE)) {

        cout << indent;
        std::string sub_indent = indent;

        if (d < domain->num_domains - 1) {
          sub_indent += "|";
          cout << "|";
        } else if (d == domain->num_domains - 1) {
          sub_indent += " ";
          cout << "'";
        }
        sub_indent += std::string(8, ' ');

        cout << "-- [" << d << "]: "
             << "(" << domain->domains[d].domain_tag << ")"
             << endl;

        print_domain(team, &domain->domains[d], sub_indent);
      }
    }
  }
}

#endif // DASH__EXAMPLES__LOCALITY__UTILS_H__INCLUDED


#include <dash/util/LocalityJSONPrinter.h>
#include <dash/util/Locality.h>
#include <dash/util/UnitLocality.h>
#include <dash/util/LocalityDomain.h>

#include <string>
#include <iostream>
#include <sstream>


namespace dash {
namespace util {


LocalityJSONPrinter & LocalityJSONPrinter::operator<<(
  const dart_hwinfo_t & hwinfo)
{
  std::ostringstream os;
  os << "{ "
     << "'numa_id':"       << hwinfo.numa_id        << ", "
     << "'num_cores':"     << hwinfo.num_cores      << ", "
     << "'core_id':"       << hwinfo.core_id        << ", "
     << "'cpu_id':"        << hwinfo.cpu_id         << ", "
     << "'threads':{"
     << "'min':"           << hwinfo.min_threads    << ","
     << "'max':"           << hwinfo.max_threads    << "}, "
     << "'cpu_mhz':{"
     << "'min':"           << hwinfo.min_cpu_mhz    << ","
     << "'max':"           << hwinfo.max_cpu_mhz    << "}, "
     << "'scopes':[";

  for (int s = 0; s < hwinfo.num_scopes; s++) {
    os << "{'"
       << hwinfo.scopes[s].scope << "':"
       << hwinfo.scopes[s].index
       << "}";
    if (s < hwinfo.num_scopes-1) {
      os << ",";
    }
  }
  os << "], ";

  os << "'cache_sizes':["  << hwinfo.cache_sizes[0] << ","
                           << hwinfo.cache_sizes[1] << ","
                           << hwinfo.cache_sizes[2] << "], "
     << "'cache_ids':["    << hwinfo.cache_ids[0]   << ","
                           << hwinfo.cache_ids[1]   << ","
                           << hwinfo.cache_ids[2]   << "], "
     << "'mem_mbps':"      << hwinfo.max_shmem_mbps // << ", "
     << " }";
  return (*this << os.str());
}

LocalityJSONPrinter & LocalityJSONPrinter::operator<<(
  dart_locality_scope_t scope)
{
  switch(scope) {
    case DART_LOCALITY_SCOPE_GLOBAL:  *this << "'GLOBAL'";    break;
    case DART_LOCALITY_SCOPE_GROUP:   *this << "'GROUP'";     break;
    case DART_LOCALITY_SCOPE_NETWORK: *this << "'NETWORK'";   break;
    case DART_LOCALITY_SCOPE_NODE:    *this << "'NODE'";      break;
    case DART_LOCALITY_SCOPE_MODULE:  *this << "'MODULE'";    break;
    case DART_LOCALITY_SCOPE_NUMA:    *this << "'NUMA'";      break;
    case DART_LOCALITY_SCOPE_UNIT:    *this << "'UNIT'";      break;
    case DART_LOCALITY_SCOPE_PACKAGE: *this << "'PACKAGE'";   break;
    case DART_LOCALITY_SCOPE_CACHE:   *this << "'CACHE'";     break;
    case DART_LOCALITY_SCOPE_CORE:    *this << "'CORE'";      break;
    default:                          *this << "'UNDEFINED'"; break;
  }
  return *this;
}

LocalityJSONPrinter & LocalityJSONPrinter::print_domain(
  dart_team_t                    team,
  const dart_domain_locality_t * domain,
  std::string                    indent)
{
  using namespace std;

  *this << "{\n";

  *this << indent << "'scope'    : " << domain->scope            << ",\n"
        << indent << "'glob_idx' : " << domain->global_index     << ",\n"
        << indent << "'rel_idx'  : " << domain->relative_index   << ",\n"
        << indent << "'level'    : " << domain->level            << ",\n"
        << indent << "'shmem'    : " << domain->shared_mem_bytes << ",\n"
        << indent << "'cores'    : " << domain->num_cores        << ",\n";

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    *this << indent << "'nodes'    : " << domain->num_domains << ",\n";
  }

  if ((static_cast<int>(domain->scope) ==
       static_cast<int>(DART_LOCALITY_SCOPE_NODE)) ||
      (static_cast<int>(domain->scope) ==
       static_cast<int>(DART_LOCALITY_SCOPE_MODULE))) {
    *this << indent << "'host'     : '"   << domain->host  << "',\n";
  }

  if (domain->num_units > 0) {
    *this << indent << "'nunits'   : " << domain->num_units << ",\n";
    *this << indent << "'unit_ids' : " << "[ ";
    for (int u = 0; u < domain->num_units; ++u) {
      *this << domain->unit_ids[u].id;
      if (u < domain->num_units-1) {
        *this << ", ";
      }
    }
    *this << " ],\n";
  }

  if (domain->scope == DART_LOCALITY_SCOPE_CORE) {
    for (int u = 0; u < domain->num_units; ++u) {
      dart_team_unit_t        unit_lid;
      dart_global_unit_t     unit_gid = domain->unit_ids[u];
      dart_unit_locality_t * uloc;

      dart_team_unit_g2l(domain->team, unit_gid, &unit_lid);
      dart_unit_locality(domain->team, unit_lid, &uloc);

      *this << indent << "'unit_id'  : { "
                      << "'local_id':"  << uloc->unit.id << ", "
                      << "'team':"      << uloc->team << ", "
                      << "'global_id':" << unit_gid.id
                      << " },\n"
            << indent << "'unit_loc' : { "
                      << "'domain':'"   << uloc->domain_tag  << "', "
                      << "'host':'"     << uloc->hwinfo.host << "',\n"
            << indent << "               'hwinfo':" << uloc->hwinfo
                      << " }";
    }
  }

  if (domain->num_domains > 0) {
    *this << indent << "'ndomains' : " << domain->num_domains << ",\n";
    *this << indent << "'domains'  : {\n";

    for (int d = 0; d < domain->num_domains; ++d) {
      if (static_cast<int>(domain->domains[d].scope) <=
          static_cast<int>(DART_LOCALITY_SCOPE_CORE)) {

        *this << indent;
        std::string sub_indent = indent;

        if (d <= domain->num_domains - 1) {
          sub_indent += " ";
          *this << " ";
        }
        sub_indent += std::string(3, ' ');

        *this << " '" << domain->domains[d].domain_tag << "' : ";

        print_domain(team, &domain->domains[d], sub_indent);

        if (d < domain->num_domains-1) {
          *this << ",";
        }
        *this << "\n";
      }
    }
    *this << indent << "}";
  }

  *this << " }";
  return *this;
}

} // namespace util
} // namespace dash

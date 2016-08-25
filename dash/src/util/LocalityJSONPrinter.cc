
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
  const typename Locality::UnitPinning & upi)
{
  std::ostringstream os;
  os << "'unit_pinning' : {"
     << "'unit': "         << upi.unit         << ", "
     << "'host': "         << upi.host         << ", "
     << "'domain': "       << upi.domain       << ", "
     << "'numa_id': "      << upi.numa_id      << ", "
     << "'cpu_id': "       << upi.cpu_id       << ", "
     << "'num_cores': "    << upi.num_cores    << ", "
     << "'num_threads': "  << upi.num_threads  << " }";
  return (*this << os.str());
}

LocalityJSONPrinter & LocalityJSONPrinter::operator<<(
  const dart_hwinfo_t & hwinfo)
{
  std::ostringstream os;
  os << "{ "
     << "'numa_id': "     << hwinfo.numa_id        << ", "
     << "'num_numa': "    << hwinfo.num_numa       << ", "
     << "'num_sockets': " << hwinfo.num_sockets    << ", "
     << "'num_cores': "   << hwinfo.num_cores      << ", "
     << "'cpu_id': "      << hwinfo.cpu_id         << ", "
     << "'threads':{ "
     << "'min': "         << hwinfo.min_threads    << ", "
     << "'max': "         << hwinfo.max_threads    << " }, "
     << "'cpu_mhz': { "
     << "'min': "         << hwinfo.min_cpu_mhz    << ","
     << "'max': "         << hwinfo.max_cpu_mhz    << " }, "
     << "'mem_mbps': "    << hwinfo.max_shmem_mbps << ", "
     << "'system_mb': "   << hwinfo.system_memory  << ", "
     << "'numa_mb': "     << hwinfo.numa_memory
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

  *this << indent << "'scope': " << domain->scope << ",\n"
        << indent << "'level': " << domain->level << ",\n"
        << indent << "'idx': "   << domain->relative_index << ",\n";

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    *this << indent << "'nodes': " << domain->num_nodes << ",\n";
  }

  if ((static_cast<int>(domain->scope) ==
       static_cast<int>(DART_LOCALITY_SCOPE_NODE)) ||
      (static_cast<int>(domain->scope) ==
       static_cast<int>(DART_LOCALITY_SCOPE_MODULE))) {
    *this << indent << "'host': "    << domain->host    << ",\n";
  }

  if (static_cast<int>(domain->scope) ==
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    *this << indent << "'node_id': " << domain->node_id << ",\n";
  }
  else if (static_cast<int>(domain->scope) >=
      static_cast<int>(DART_LOCALITY_SCOPE_NUMA)) {
    *this << indent << "'numa_id': " << domain->hwinfo.numa_id  << ",\n";
  }

  if (domain->num_units > 0) {
    *this << indent << "'units': " << "[ ";
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t g_unit_id;
      dart_team_unit_l2g(domain->team, domain->unit_ids[u], &g_unit_id);
      *this << g_unit_id;
      if (u < domain->num_units-1) {
        *this << ", ";
      }
    }
    *this << " ],\n";
  }

  if (domain->scope == DART_LOCALITY_SCOPE_CORE) {
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      *this << indent << "'unit_id': { "
                      << "'local_id': "  << uloc->unit << ", "
                      << "'team': "      << uloc->team << ", "
                      << "'global_id': " << unit_gid
                      << " }, \n"
            << indent << "'unit_locality': { "
                      << "'domain': '"   << uloc->domain_tag << "', "
                      << "'host': '"     << uloc->host       << "', "
                      << "'hwinfo': "    << uloc->hwinfo
                      << " } ";
    }
  } else {
    *this << indent << "'hwinfo': " << domain->hwinfo << " ";
  }

  if (domain->num_domains > 0) {
    *this << ",\n";
    *this << indent << "'ndomains': " << domain->num_domains << ",\n";
    *this << indent << "'domains': {\n";

    for (int d = 0; d < domain->num_domains; ++d) {
      if (static_cast<int>(domain->domains[d].scope) <=
          static_cast<int>(DART_LOCALITY_SCOPE_CORE)) {

        *this << indent;
        std::string sub_indent = indent;

        if (d < domain->num_domains - 1) {
          sub_indent += " ";
          *this << " ";
        } else if (d == domain->num_domains - 1) {
          sub_indent += " ";
          *this << " ";
        }
        sub_indent += std::string(8, ' ');

        *this << "    '" << domain->domains[d].domain_tag << "' : ";

        print_domain(team, &domain->domains[d], sub_indent);

        if (d < domain->num_domains-1) {
          *this << ",";
        }
        *this << "\n";
      }
    }
    *this << indent << " }";
  }

  *this << " }";
  return *this;
}

} // namespace util
} // namespace dash

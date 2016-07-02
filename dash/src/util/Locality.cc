#include <dash/util/Locality.h>

#include <dash/Array.h>
#include <dash/algorithm/Copy.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>


namespace dash {
namespace util {

static void print_domain(
  std::ostream                 & ostr,
  dart_team_t                    team,
  const dart_domain_locality_t * domain,
  std::string                    indent = "");

} // namespace util
} // namespace dash

std::ostream & operator<<(
  std::ostream                 & os,
  const dart_domain_locality_t & domain_loc)
{
  dash::util::print_domain(os, domain_loc.team, &domain_loc);
  return os;
}

std::ostream & operator<<(
  std::ostream                 & os,
  const dart_unit_locality_t   & unit_loc)
{
  std::ostringstream ss;
  ss << "dart_unit_locality_t("
     <<   "unit:"      << unit_loc.unit               << " "
     <<   "domain:'"   << unit_loc.domain_tag         << "' "
     <<   "host:'"     << unit_loc.host               << "' "
     <<   "numa_id:'"  << unit_loc.hwinfo.numa_id     << "' "
     <<   "core_id:"   << unit_loc.hwinfo.cpu_id      << " "
     <<   "n_cores:"   << unit_loc.hwinfo.num_cores   << " "
     <<   "cpu_mhz:"   << unit_loc.hwinfo.min_cpu_mhz << ".."
                       << unit_loc.hwinfo.max_cpu_mhz << " "
     <<   "threads:"   << unit_loc.hwinfo.max_threads << " "
     <<   "mem_mbps:"  << unit_loc.hwinfo.max_shmem_mbps
     << ")";
  return operator<<(os, ss.str());
}

std::ostream & operator<<(
  std::ostream                 & os,
  dash::util::Locality::Scope    scope)
{
  return os << (static_cast<dart_locality_scope_t>(scope));
}

std::ostream & operator<<(
  std::ostream                 & os,
  dart_locality_scope_t          scope)
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

namespace dash {
namespace util {

void Locality::init()
{
  DASH_LOG_DEBUG("dash::util::Locality::init()");

  if (dart_unit_locality(DART_TEAM_ALL, dash::myid(), &_unit_loc)
      != DART_OK) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_unit_locality failed " <<
               "for unit " << dash::myid());
  }
  DASH_LOG_TRACE_VAR("dash::util::Locality::init", _unit_loc);
  if (_unit_loc == nullptr) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_unit_locality returned nullptr " <<
               "for unit " << dash::myid());
  }

  if (dart_domain_team_locality(
        DART_TEAM_ALL, _unit_loc->domain_tag, &_domain_loc)
      != DART_OK) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_domain_locality failed " <<
               "for domain '" << _unit_loc->domain_tag << "'");
  }
  DASH_LOG_TRACE_VAR("dash::util::Locality::init", _domain_loc);
  if (_domain_loc == nullptr) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_domain_locality returned nullptr " <<
               "for domain '" << _unit_loc->domain_tag << "'");
  }

  _cache_sizes[0]      = _domain_loc->hwinfo.cache_sizes[0];
  _cache_sizes[1]      = _domain_loc->hwinfo.cache_sizes[1];
  _cache_sizes[2]      = _domain_loc->hwinfo.cache_sizes[2];
  _cache_line_sizes[0] = _domain_loc->hwinfo.cache_line_sizes[0];
  _cache_line_sizes[1] = _domain_loc->hwinfo.cache_line_sizes[1];
  _cache_line_sizes[2] = _domain_loc->hwinfo.cache_line_sizes[2];

  if (_cache_line_sizes[0] < 0) {
    _cache_line_sizes[0] = 64;
  }
  if (_cache_line_sizes[1] < 0) {
    _cache_line_sizes[1] = _cache_line_sizes[0];
  }
  if (_cache_line_sizes[2] < 0) {
    _cache_line_sizes[2] = _cache_line_sizes[1];
  }

  DASH_LOG_DEBUG("dash::util::Locality::init >");
}

std::ostream & operator<<(
  std::ostream        & os,
  const typename Locality::UnitPinning & upi)
{
  os << "dash::util::Locality::UnitPinning("
     << "unit:"         << upi.unit         << " "
     << "host:"         << upi.host         << " "
     << "domain:"       << upi.domain       << " "
     << "numa_id:"      << upi.numa_id      << " "
     << "cpu_id:"       << upi.cpu_id       << " "
     << "num_cores:"    << upi.num_cores    << " "
     << "num_threads:"  << upi.num_threads  << ")";
  return os;
}

std::ostream & operator<<(
  std::ostream        & os,
  const dart_hwinfo_t & hwinfo)
{
  os << "dart_hwinfo_t("
     << "numa_id:"     << hwinfo.numa_id     << " "
     << "num_numa:"    << hwinfo.num_numa    << " "
     << "num_sockets:" << hwinfo.num_sockets << " "
     << "num_cores:"   << hwinfo.num_cores   << " "
     << "cpu_id:"      << hwinfo.cpu_id      << " "
     << "threads("     << hwinfo.min_threads << "..."
                       << hwinfo.max_threads << ") "
     << "cpu_mhz("     << hwinfo.min_cpu_mhz << "..."
                       << hwinfo.max_cpu_mhz << ") "
     << "mem_mbps:"    << hwinfo.max_shmem_mbps
     << ")";
  return os;
}

dart_unit_locality_t   * Locality::_unit_loc   = nullptr;
dart_domain_locality_t * Locality::_domain_loc = nullptr;

std::array<int, 3> Locality::_cache_sizes;
std::array<int, 3> Locality::_cache_line_sizes;

static void print_domain(
  std::ostream                 & ostr,
  dart_team_t                    team,
  const dart_domain_locality_t * domain,
  std::string                    indent)
{
  using namespace std;

  ostr << indent << "scope:   " << domain->scope << " "
                                << "(level " << domain->level << ")"
       << '\n'
       << indent << "rel.idx: " << domain->relative_index
       << '\n';

  if (static_cast<int>(domain->scope) <
      static_cast<int>(DART_LOCALITY_SCOPE_NODE)) {
    ostr << indent << "nodes:   " << domain->num_nodes << '\n';
  }

  if (static_cast<int>(domain->scope) >=
      static_cast<int>(DART_LOCALITY_SCOPE_NUMA)) {
    ostr << indent << "NUMA id: " << domain->hwinfo.numa_id  << '\n';
  }

  if (domain->num_units > 0) {
    ostr << indent << "units:   " << "{ ";
    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t g_unit_id;
      dart_team_unit_l2g(domain->team, domain->unit_ids[u], &g_unit_id);
      ostr << g_unit_id;
      if (u < domain->num_units-1) {
        ostr << ", ";
      }
    }
    ostr << " }" << '\n';
  }

  if (domain->scope == DART_LOCALITY_SCOPE_CORE) {
    std::string uindent = indent;
    uindent += std::string(9, ' ');

    for (int u = 0; u < domain->num_units; ++u) {
      dart_unit_t            unit_id  = domain->unit_ids[u];
      dart_unit_t            unit_gid = DART_UNDEFINED_UNIT_ID;
      dart_unit_locality_t * uloc;
      dart_unit_locality(team, unit_id, &uloc);
      dart_team_unit_l2g(uloc->team, unit_id, &unit_gid);
      ostr << uindent << "unit id:   " << uloc->unit << "  ("
                                       << "in team " << uloc->team << ", "
                                       << "global: " << unit_gid   << ")"
                      << '\n';
      ostr << uindent << "domain:    " << uloc->domain_tag << '\n';
      ostr << uindent << "host:      " << uloc->host       << '\n';
      ostr << uindent << "hwinfo:    " << uloc->hwinfo     << '\n';
    }
  } else {
    ostr << indent << "hwinfo:  " << domain->hwinfo << '\n';
  }

  if (domain->num_domains > 0) {
    ostr << indent << "domains: " << domain->num_domains << '\n';

    for (int d = 0; d < domain->num_domains; ++d) {
      if (static_cast<int>(domain->domains[d].scope) <=
          static_cast<int>(DART_LOCALITY_SCOPE_CORE)) {

        ostr << indent;
        std::string sub_indent = indent;

        if (d < domain->num_domains - 1) {
          sub_indent += "|";
          ostr << "|";
        } else if (d == domain->num_domains - 1) {
          sub_indent += " ";
          ostr << "'";
        }
        sub_indent += std::string(8, ' ');

        ostr << "-- [" << d << "]: "
             << "(" << domain->domains[d].domain_tag << ")"
             << '\n';

        print_domain(ostr, team, &domain->domains[d], sub_indent);
      }
    }
  }
}

} // namespace util
} // namespace dash

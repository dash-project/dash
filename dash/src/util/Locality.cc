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


std::ostream & operator<<(
  std::ostream                 & os,
  const dart_domain_locality_t & domain_loc)
{
  std::ostringstream ss;
  ss << "dart_domain_locality_t("
     <<   "level:"     << domain_loc.level              << " "
     <<   "scope:"     << domain_loc.scope              << " "
     <<   "n_nodes:"   << domain_loc.num_nodes          << " "
     <<   "n_numa:"    << domain_loc.hwinfo.num_numa    << " "
     <<   "n_sockets:" << domain_loc.hwinfo.num_sockets << " "
     <<   "n_cores:"   << domain_loc.hwinfo.num_cores   << " "
     <<   "cpu_mhz:"   << domain_loc.hwinfo.min_cpu_mhz << ".."
                       << domain_loc.hwinfo.max_cpu_mhz << " "
     <<   "n_domains:" << domain_loc.num_domains
     << ")";
  return operator<<(os, ss.str());
}

std::ostream & operator<<(
  std::ostream               & os,
  const dart_unit_locality_t & unit_loc)
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
     <<   "threads:"   << unit_loc.hwinfo.max_threads
     << ")";
  return operator<<(os, ss.str());
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

  if (dart_domain_locality(DART_TEAM_ALL, _unit_loc->domain_tag, &_domain_loc)
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

  DASH_LOG_DEBUG("dash::util::Locality::init >");
}

std::ostream & operator<<(
  std::ostream        & os,
  const typename Locality::UnitPinning & upi)
{
  std::ostringstream ss;
  ss << "dash::util::Locality::UnitPinning("
     << "unit:"         << upi.unit         << " "
     << "host:"         << upi.host         << " "
     << "domain:"       << upi.domain       << " "
     << "numa_id:"      << upi.numa_id      << " "
     << "core_id:"      << upi.cpu_id       << " "
     << "num_cores:"    << upi.num_cores    << " "
     << "max_threads:"  << upi.num_threads  << ")";
  return operator<<(os, ss.str());
}

dart_unit_locality_t   * Locality::_unit_loc   = nullptr;
dart_domain_locality_t * Locality::_domain_loc = nullptr;

std::array<int, 3> Locality::_cache_sizes;
std::array<int, 3> Locality::_cache_line_sizes;

} // namespace util
} // namespace dash

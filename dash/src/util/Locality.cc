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

void Locality::init()
{
  if (dart_unit_locality(dash::myid(), &_unit_loc) != DART_OK) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_unit_locality failed " <<
               "for unit " << dash::myid());
  }
  if (_unit_loc == nullptr) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_unit_locality returned nullptr " <<
               "for unit " << dash::myid());
  }
  if (dart_domain_locality(_unit_loc->domain_tag, &_domain_loc) != DART_OK) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_domain_locality failed " <<
               "for domain '" << _unit_loc->domain_tag << "'");
  }
  if (_domain_loc == nullptr) {
    DASH_THROW(dash::exception::RuntimeError,
               "Locality::init(): dart_domain_locality returned nullptr " <<
               "for domain '" << _unit_loc->domain_tag << "'");
  }

  _cache_sizes[0]      = _domain_loc->cache_sizes[0];
  _cache_sizes[1]      = _domain_loc->cache_sizes[1];
  _cache_sizes[2]      = _domain_loc->cache_sizes[2];
  _cache_line_sizes[0] = _domain_loc->cache_line_sizes[0];
  _cache_line_sizes[1] = _domain_loc->cache_line_sizes[1];
  _cache_line_sizes[2] = _domain_loc->cache_line_sizes[2];

  // Collect process pinning information:
  dash::Array<UnitPinning> pinning(dash::size());

  int cpu       = dash::util::Locality::UnitCPUIds()[0];
  int numa_node = dash::util::Locality::UnitNUMANodes()[0];

  UnitPinning my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  strncpy(my_pin_info.host,
          _domain_loc->host,
          DART_LOCALITY_HOST_MAX_SIZE);

  pinning[dash::myid()] = my_pin_info;

  // Ensure pinning data is ready:
  dash::barrier();

  // TODO:  Change to directly copying to local_vector.begin() once
  //        dash::copy is available for iterator output ranges
  //
  // Create local copy of pinning info:

  // Copy into temporary array:
  UnitPinning * local_copy_tmp = new UnitPinning[pinning.size()];

  auto copy_end = dash::copy(pinning.begin(), pinning.end(),
                             local_copy_tmp);
  auto n_copied = copy_end - local_copy_tmp;
  DASH_LOG_TRACE_VAR("dash::util::Locality::init", n_copied);
  // Copy from temporary array to local vector:
  _unit_pinning.insert(_unit_pinning.end(),
                       local_copy_tmp, local_copy_tmp + n_copied);
  // Free temporary array:
  delete[] local_copy_tmp;

  // Wait for completion of the other units' copy operations:
  dash::barrier();
}

std::ostream & operator<<(
  std::ostream        & os,
  const typename Locality::UnitPinning & upi)
{
  std::ostringstream ss;
  ss << "dash::util::Locality::UnitPinning("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

dart_unit_locality_t   * Locality::_unit_loc   = nullptr;
dart_domain_locality_t * Locality::_domain_loc = nullptr;

std::vector<Locality::UnitPinning> Locality::_unit_pinning;

std::array<int, 3> Locality::_cache_sizes;
std::array<int, 3> Locality::_cache_line_sizes;

} // namespace util
} // namespace dash

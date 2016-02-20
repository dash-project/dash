#include <dash/util/Locality.h>

#include <dash/Array.h>
#include <dash/algorithm/Copy.h>

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
  // Collect process pinning information:
  dash::Array<UnitPinning> pinning(dash::size());

  int cpu       = dash::util::Locality::UnitCPU();
  int numa_node = dash::util::Locality::UnitNUMANode();

  UnitPinning my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  gethostname(my_pin_info.host, 100);

  pinning[dash::myid()] = my_pin_info;

  // Ensure pinning data is ready:
  dash::barrier();

  // Create local copy of pinning info:

  // TODO:
  // Change to directly copying to local_vector.begin()
  // when dash::copy is available for iterator output
  // ranges.

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

std::vector<Locality::UnitPinning> Locality::_unit_pinning;

} // namespace util
} // namespace dash

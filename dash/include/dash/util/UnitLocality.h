#ifndef DASH__UTIL__UNIT_LOCALITY_H__INCLUDED
#define DASH__UTIL__UNIT_LOCALITY_H__INCLUDED

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>
#include <dash/util/Config.h>

#include <dash/algorithm/internal/String.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/Exception.h>
#include <dash/Team.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <algorithm>


namespace dash {
namespace util {

/**
 * Wrapper of a single \c dart_unit_locality_t object.
 */
class UnitLocality
{
private:
  typedef UnitLocality  self_t;

public:

  UnitLocality(
    dash::Team  & team,
    dart_unit_t   unit)
  : _team(&team)
  {
    DASH_ASSERT_RETURNS(
      dart_unit_locality(
        team.dart_id(), unit, &_unit_locality),
      DART_OK);
  }

  UnitLocality()                                 = default;
  UnitLocality(const UnitLocality &)             = default;
  UnitLocality & operator=(const UnitLocality &) = default;

  inline const dart_hwinfo_t & hwinfo() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return _unit_locality->hwinfo;
  }

  inline dart_hwinfo_t & hwinfo()
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return _unit_locality->hwinfo;
  }

  inline dash::Team & team()
  {
    if (nullptr == _team) {
      return dash::Team::Null();
    }
    return *_team;
  }

  inline dart_unit_t unit_id() const
  {
    return nullptr == _unit_locality
           ? DART_UNDEFINED_UNIT_ID
           : _unit_locality->unit;
  }

  inline std::string domain_tag() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return _unit_locality->domain_tag;
  }

  inline std::string host() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return _unit_locality->host;
  }

  inline void set_domain_tag(
    const std::string & tag)
  {
    strcpy(_unit_locality->domain_tag, tag.c_str());
  }

  inline void set_host(
    const std::string & hostname)
  {
    strcpy(_unit_locality->host, hostname.c_str());
  }

  inline int num_cores() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return (_unit_locality->hwinfo.num_cores);
  }

  inline int num_threads() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return (dash::util::Config::get<bool>("DASH_MAX_SMT")
               ? _unit_locality->hwinfo.max_threads
               : _unit_locality->hwinfo.min_threads);
  }

  inline int cpu_mhz() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return (_unit_locality->hwinfo.max_cpu_mhz);
  }

  inline int max_shmem_mbps() const
  {
    DASH_ASSERT(nullptr != _unit_locality);
    return (_unit_locality->hwinfo.max_shmem_mbps);
  }

private:

  dash::Team             * _team          = nullptr;
  dart_unit_locality_t   * _unit_locality = nullptr;

}; // class UnitLocality

} // namespace util
} // namespace dash

#endif // DASH__UTIL__UNIT_LOCALITY_H__INCLUDED

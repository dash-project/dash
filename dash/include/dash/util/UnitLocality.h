#ifndef DASH__UTIL__UNIT_LOCALITY_H__INCLUDED
#define DASH__UTIL__UNIT_LOCALITY_H__INCLUDED

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>

#include <dash/algorithm/internal/String.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

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
  : _team(team)
  {
    DASH_ASSERT_RETURNS(
      dart_unit_locality(
        team.dart_id(), unit, &_unit_locality),
      DART_OK);
  }

  UnitLocality() = delete;

  inline const dart_hwinfo_t & hwinfo() const
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _unit_locality->hwinfo;
  }

  inline dash::Team & team()
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _team;
  }

  inline dart_unit_t unit_id() const
  {
    return _unit_locality == nullptr
           ? DART_UNDEFINED_UNIT_ID
           : _unit_locality->unit;
  }

  inline std::string domain_tag() const
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _unit_locality->domain_tag;
  }

  inline std::string host() const
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _unit_locality->host;
  }

private:
  dash::Team             & _team;
  dart_unit_locality_t   * _unit_locality = nullptr;

}; // class UnitLocality

} // namespace util
} // namespace dash

#endif // DASH__UTIL__UNIT_LOCALITY_H__INCLUDED

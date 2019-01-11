#ifndef DASH__UTIL__LOCALITY_JSON_PRINTER_H__INCLUDED
#define DASH__UTIL__LOCALITY_JSON_PRINTER_H__INCLUDED

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>
#include <dash/util/UnitLocality.h>
#include <dash/util/TeamLocality.h>

#include <string>
#include <iostream>
#include <sstream>



namespace dash {
namespace util {

class LocalityJSONPrinter
{
private:

  typedef LocalityJSONPrinter self_t;

public:
  LocalityJSONPrinter() = default;

  self_t & operator<<(const std::string & str) {
    _os << str;
    return *this;
  }

  self_t & operator<<(const dart_unit_locality_t & unit_loc);

  self_t & operator<<(
    const dart_hwinfo_t & hwinfo);

  self_t & operator<<(
    const dart_domain_locality_t & domain_loc) {
    return print_domain(domain_loc.team, &domain_loc, "");
  }

  self_t & operator<<(
    dart_locality_scope_t scope);

  self_t & operator<<(
    dash::util::Locality::Scope scope) {
    return *this << (static_cast<dart_locality_scope_t>(scope));
  }

  std::string str() const {
    return _os.str();
  }

private:

  self_t & print_domain(
    dart_team_t                    team,
    const dart_domain_locality_t * domain,
    std::string                    indent);

private:

  std::ostringstream _os;
};

template<typename T>
typename std::enable_if<
  std::is_integral<T>::value,
  LocalityJSONPrinter & >::type
operator<<(
  LocalityJSONPrinter & ljp,
  const T & v) {
  std::ostringstream os;
  os << v;
  ljp << os.str();
  return ljp;
}

} // namespace dash
} // namespace util


#endif // DASH__UTIL__LOCALITY_JSON_PRINTER_H__INCLUDED

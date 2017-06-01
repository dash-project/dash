#ifndef DASH__INTERNAL__STREAM_CONVERSION_H_
#define DASH__INTERNAL__STREAM_CONVERSION_H_

#include <dash/internal/Macro.h>

#include <dash/meta/TypeInfo.h>

#include <dash/dart/if/dart_types.h>

#include <array>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <sstream>
#include <iterator>
#include <cstring>
#include <memory>
#include <type_traits>


namespace dash {

std::ostream & operator<<(
  std::ostream & o,
  dart_global_unit_t uid);

std::ostream & operator<<(
  std::ostream & o,
  dart_team_unit_t uid);


/**
 * Write \c std::shared_ptr to output stream.
 */
template <class T>
std::ostream & operator<<(
  std::ostream              & os,
  const std::shared_ptr<T> & p) {
  os << dash::typestr(p)
     << "(" << p.get() << ")";
  return os;
}

/**
 * Write \c std::unique_ptr to output stream.
 */
template <class T>
std::ostream & operator<<(
  std::ostream              & os,
  const std::unique_ptr<T> & p) {
  os << dash::typestr(p)
     << "(" << p.get() << ")";
  return os;
}

/**
 * Write \c std::pair to output stream.
 */
template <class T1, class T2>
std::ostream & operator<<(
  std::ostream            & os,
  const std::pair<T1, T2> & p) {
  os << "(" << p.first << "," << p.second << ")";
  return os;
}


/**
 * Write elements in std::map to output stream.
 */
template <typename T1, typename T2>
std::ostream & operator<<(
  std::ostream & o,
  const std::map<T1,T2> & map)
{
  std::ostringstream ss;
  auto nelem = map.size();
  ss << "{ ";
  int i = 1;
  for (auto kv : map) {
    ss << "(" << kv.first
       << ":" << kv.second << ")"
       << (i++ < nelem ? ", " : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}

/**
 * Write elements in std::set to output stream.
 */
template <typename T>
std::ostream & operator<<(
  std::ostream & o,
  const std::set<T> & set)
{
  std::ostringstream ss;
  auto nelem = set.size();
  ss << "{ ";
  int i = 1;
  for (auto e : set) {
    ss << e
       << (i++ < nelem ? ", " : "");
  }
  ss << " }";
  operator<<(o, ss.str());
  return o;
}

} // namespace dash

#endif // DASH__INTERNAL__STREAM_CONVERSION_H_

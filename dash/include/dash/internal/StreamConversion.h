#ifndef DASH__INTERNAL__STREAM_CONVERSION_H_
#define DASH__INTERNAL__STREAM_CONVERSION_H_

#include <dash/internal/Macro.h>

#include <dash/meta/TypeInfo.h>
#include <dash/Range.h>

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

/**
 * Write range of random access iterators to output stream.
 */
template <
  class Range,
  class RangeDType = typename std::decay<Range>::type
>
auto operator<<(
  std::ostream  & o,
  Range        && range)
  -> typename std::enable_if<
       (
      // type is range:
         dash::is_range<RangeDType>::value &&
      // type is not std::string or derivative:
         !std::is_same<RangeDType, std::string>::value &&
         !std::is_base_of<std::string, RangeDType>::value &&
      // range iterator type is random access:
         std::is_same<
           std::random_access_iterator_tag,
           typename std::iterator_traits<
             typename std::decay<
               decltype(dash::begin(std::forward<Range>(range)))
             >::type
           >::iterator_category
         >::value
       ),
       std::ostream &
    >::type
{
  typedef typename std::iterator_traits<decltype(range.begin())>::value_type
    value_t;

  auto && rng = std::forward<Range>(range);

  std::ostringstream ss;
  int pos = 0;
  ss << dash::typestr(*dash::begin(rng))
     << " { ";
  for (auto it = dash::begin(rng); it != dash::end(rng); ++it, ++pos) {
    ss << static_cast<const value_t>(*it) << " ";
  }
  ss << "}";
  return operator<<(o, ss.str());
}

/**
 * Write range of non-random access iterators to output stream.
 */
template <
  class Range,
  class RangeDType = typename std::decay<Range>::type >
auto operator<<(
  std::ostream  & o,
  Range        && range)
  -> typename std::enable_if<
       (
      // type is range:
         dash::is_range<RangeDType>::value &&
      // type is not std::string or derivative:
         !std::is_same<RangeDType, std::string>::value &&
         !std::is_base_of<std::string, RangeDType>::value &&
      // range iterator type is not random access:
         !std::is_same<
           std::random_access_iterator_tag,
           typename std::iterator_traits<
             typename std::decay<
               decltype(dash::begin(std::forward<Range>(range)))
             >::type
           >::iterator_category
         >::value
       ),
       std::ostream &
    >::type
{
  typedef typename std::iterator_traits<
                     decltype(range.begin())
                   >::value_type
    value_t;

  auto && rng = std::forward<Range>(range);

  std::ostringstream ss;
  ss << dash::typestr(*dash::begin(rng))
     << " { ";
  for (auto it = dash::begin(rng); it != dash::end(rng); ++it) {
    ss << static_cast<const value_t>(*it) << " ";
  }
  ss << "}";
  return operator<<(o, ss.str());
}


} // namespace dash

#endif // DASH__INTERNAL__STREAM_CONVERSION_H_

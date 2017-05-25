#ifndef DASH__VIEW__MAKE_RANGE_H__INCLUDED
#define DASH__VIEW__MAKE_RANGE_H__INCLUDED

#include <dash/Range.h>
#include <dash/view/Sub.h>


namespace dash {

#if 1
/**
 * Adapter utility function.
 * Wraps `begin` and `end` pointers in range type.
 */
template <
  class    Iterator,
  class    Sentinel,
  typename std::enable_if< std::is_pointer<Iterator>::value > * = nullptr >
constexpr dash::IteratorRange<Iterator *, Sentinel *>
make_range(
  Iterator * const begin,
  Sentinel * const end) {
  return dash::IteratorRange<Iterator *, Sentinel *>(
           begin,
           end);
}
#endif

#if 0
/**
 * Adapter utility function.
 * Wraps `begin` and `end` pointers in range type.
 */
template <class Iterator, class Sentinel>
constexpr dash::IteratorRange<Iterator *, Sentinel *>
make_range(
  Iterator * begin,
  Sentinel * end) {
  return dash::IteratorRange<Iterator *, Sentinel *>(
           begin,
           end);
}
#endif

#if 1
/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <
  class    Iterator,
  class    Sentinel,
  typename std::enable_if< !std::is_pointer<Iterator>::value > * = nullptr >
auto
make_range(
  const Iterator & begin,
  const Sentinel & end)
  -> decltype(dash::sub(
                begin.pos(), end.pos(),
                dash::IteratorRange<Iterator, Sentinel>(
                  begin - begin.pos(), end))) {
  return dash::sub(begin.pos(), end.pos(),
                   dash::IteratorRange<Iterator, Sentinel>(
                     begin - begin.pos(), end));
}
#else
/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <class Iterator, class Sentinel>
dash::IteratorRange<Iterator, Sentinel>
make_range(
  const Iterator & begin,
  const Sentinel & end) {
  return dash::IteratorRange<Iterator, Sentinel>(begin, end);
}

/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <class Iterator, class Sentinel>
dash::IteratorRange<
  typename std::decay<Iterator>::type,
  typename std::decay<Sentinel>::type >
make_range(
  Iterator && begin,
  Sentinel && end) {
  return dash::IteratorRange<
           typename std::decay<Iterator>::type,
           typename std::decay<Sentinel>::type
         >(std::forward<Iterator>(begin),
           std::forward<Sentinel>(end));
}
#endif

#if 0
/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <
  class    Iterator,
  class    Sentinel,
  typename std::enable_if< !std::is_pointer<Iterator>::value > * = nullptr >
auto
make_range(
  Iterator && begin,
  Sentinel && end)
  -> decltype(dash::sub(begin.pos(), end.pos(),
                        dash::IteratorRange<
                          typename std::decay<Iterator>::type,
                          typename std::decay<Sentinel>::type
                        >((begin) -= begin.pos(),
                          (end)   -= begin.pos()))) {
  return dash::sub(begin.pos(), end.pos(),
                   dash::IteratorRange<
                     typename std::decay<Iterator>::type,
                     typename std::decay<Sentinel>::type
                   >((begin) -= begin.pos(),
                     (end)   -= begin.pos()));
}
#endif

} // namespace dash

#endif // DASH__VIEW__MAKE_RANGE_H__INCLUDED

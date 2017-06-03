#ifndef DASH__COARRAY_RANGES_H__INCLUDED
#define DASH__COARRAY_RANGES_H__INCLUDED

/**
 * \defgroup  DashRangeConcept  Multidimensional Range Concept
 *
 * \ingroup DashCoarrayConcept 
 * \{
 * \par Description
 *
 * The Coarray Range extends \c dash::IteratorRange by providing a
 * random access operator
 *
 * \}
 */


#include <dash/Range.h>


namespace dash {
namespace coarray {

/**
 * Adapter template for range concept, wraps `begin` and `end` iterators
 * in range type and provides random access operator.
 */
template <
  typename Iterator,
  typename Sentinel >
class IteratorRange
: public ::dash::IteratorRange<Iterator, Sentinel>
{

private:
  using value_reference = typename Iterator::reference;
public:

constexpr IteratorRange(Iterator & begin, Sentinel & end):
  ::dash::IteratorRange<Iterator, Sentinel>(begin, end) {}

constexpr value_reference operator[](const typename Iterator::index_type & i){
  return this->begin()[i];
}
};

/**
 * Adapter utility function.
 * Wraps `begin` and `end` const iterators in range type.
 */
template <class Iterator, class Sentinel>
constexpr dash::coarray::IteratorRange<const Iterator, const Sentinel>
make_range(
  const Iterator & begin,
  const Sentinel & end) {
  return dash::coarray::IteratorRange<const Iterator, const Sentinel>(
           begin,
           end);
}

/**
 * Adapter utility function.
 * Wraps `begin` and `end` iterators in range type.
 */
template <class Iterator, class Sentinel>
dash::coarray::IteratorRange<Iterator, Sentinel>
make_range(
  Iterator & begin,
  Sentinel & end) {
  return dash::coarray::IteratorRange<Iterator, Sentinel>(
           begin,
           end);
}

} // namespace coarray
} // namespace dash

#endif // DASH__COARRAY_RANGES_H__INCLUDED

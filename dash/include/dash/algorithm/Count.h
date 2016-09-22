#ifndef DASH__ALGORITHM__COUNT_H__
#define DASH__ALGORITHM__COUNT_H__

#include <dash/Array.h>
#include <dash/iterator/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/algorithm/Operation.h>
#include <dash/dart/if/dart_communication.h>

namespace dash {

/*
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> count(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Value which will be assigned to the elements in range [first, last)
  const ElementType                  & value)
{
  typedef dash::default_index_t index_t;

  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first, last);
  auto l_first       = index_range.begin;
  auto l_last        = index_range.end;

  auto l_result      = std::count(l_first, l_last, value);

  dash::Array<int> l_results(dash::size());

  l_results.local[0] = l_result;

  dash::barrier();
  int occurencies;

  // wie verwende ich die reduce Funktion?
  dash::reduce(l_results, occurencies)

  return occurencies;
}

/**
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType,
	class    UnaryPredicate >
GlobIter<ElementType, PatternType> find_if(
  /// Iterator to the initial position in the sequence
  GlobIter<ElementType, PatternType>   first,
  /// Iterator to the final position in the sequence
  GlobIter<ElementType, PatternType>   last,
  /// Predicate which will be applied to the elements in range [first, last)
	UnaryPredicate                       predicate)
{
  typedef dash::default_index_t index_t;

  auto myid          = dash::myid();
  /// Global iterators to local range:
  auto index_range   = dash::local_range(first, last);
  auto l_first       = index_range.begin;
  auto l_last        = index_range.end;

  auto l_result      = std::count_if(l_first, l_last, predicate);

  dash::Array<dart_unit_t> l_results(dash::size());

  l_results.local[0] = l_offset;

  dash::barrier();

  // All local offsets stored in l_results
  int occurrencies;
  //wie verwendet man das?
  dash::reduce(l_results, occurrencies);
  return last;
}

} // namespace dash

#endif // DASH__ALGORITHM__COUNT_H__

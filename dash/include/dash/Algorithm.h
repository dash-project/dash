#ifndef DASH__ALGORITHM_H_
#define DASH__ALGORITHM_H_

#include <algorithm>
#include <functional>
#include <cstddef>

namespace dash {

template<typename ElementType>
struct LocalRange {
  ElementType * begin;
  ElementType * end;
} LocalRange;

/**
 * Resolve the number of elements between two global iterators.
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(d)
 */
template<typename ElementType>
gptrdiff_t distance(
  /// Iterator to the initial position in the global sequence
  const GlobIter<ElementType> & first,
  /// Iterator to the final position in the global sequence
  const GlobIter<ElementType> & last) {
  return last - first;
}

/**
 * Resolves the local address range between global iterators:
 *
 * \return      A local range consisting of native pointers to the first
 *              and last local element within the sequence limited by the
 *              given global iterators.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d)
 */
template<typename ElementType>
LocalRange<ElementType> local_subrange(
  /// Iterator to the initial position in the global sequence
  const GlobIter<ElementType> & first,
  /// Iterator to the final position in the global sequence
  const GlobIter<ElementType> & last) {
  auto begin_gindex   = first.pos();
  auto end_gindex     = last.pos();
  // Get pattern from global iterators, O(1):
  auto pattern        = first.pattern();
  // Global index of first element in pattern, O(1):
  auto lbegin_gindex  = pattern.lbegin();
  // Global index of last element in pattern, O(1):
  auto lend_gindex    = pattern.lend();
  // Intersect local range and global range, in global index domain:
  auto goffset_lbegin = std::max(lbegin_gindex, begin_gindex);
  auto goffset_lend   = std::min(lend_gindex, end_gindex);
  // Global positions of local range to global coordinates, O(d):
  auto lbegin_gcoords = pattern.coords(goffset_lbegin);
  auto lend_gcoords   = pattern.coords(goffset_lend);
  // Global coordinates of local range to local indices, O(d):
  auto lbegin_index   = pattern.index_to_elem(lbegin_gcoords);
  auto lend_index     = pattern.index_to_elem(lend_gcoords);
  // Local start address from global memory:
  auto lbegin         = first.globmem().lbegin();
  // Add local offsets to local start address:
  if (lbegin == nullptr) {
    return LocalRange<ElementType> { nullptr, nullptr };
  }
  return LocalRange<ElementType> { lbegin + lbegin_index,
                                   lbegin + lend_index };
}

/**
 * Invoke a function on every element in a range distributed by a pattern.
 * Being a collaborative operation, each unit will invoke the given
 * function on its local elements only.
 *
 * \tparam      PatternIterator  Type of the global iterator specifying the
 *                               element range to scan, implementing the
 *                               PatternInterator concept
 * \tparam      IndexType        Parameter type expected by function to
 *                               invoke, deduced from parameter \c func
 * \complexity  O(n)
 */
template<typename PatternIterator, typename IndexType>
void for_each(
  /// Iterator to the initial position in the sequence
  const PatternIterator begin,
  /// Iterator to the final position in the sequence
  const PatternIterator end,
  /// Function to invoke on every index in the range
  ::std::function<void(IndexType)> func) {
  auto range   = end - begin;
  auto pattern = begin.pattern();
  for (IndexType i = 0;
       i < pattern.size() && range > 0;
       ++i, --range) {
    IndexType idx = pattern.local_to_global_index(
      pattern.team().myid(), i);
    if (idx < 0) {
      break;
    }
    func(idx);
  }
}

/**
 * Returns an iterator pointing to the element with the smallest value in
 * the range [first,last).
 *
 * \return  An iterator to smallest value in the range, or last if the 
 *          range is empty.
 *
 * \tparam  ElementType      Type of the elements in the sequence
 * \tparam  PatternIterator  Type of the global iterator specifying the
 *                           element sequence to scan, implementing the
 *                           PatternInterator concept
 * \complexity  O(n)
 */
template<typename ElementType>
GlobPtr<ElementType> min_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType> first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType> last) {
  typedef dash::GlobPtr<ElementType> globptr_t;
  auto pattern         = first.pattern();
  dash::Team & team    = pattern.team();
  dash::Array<globptr_t> minarr(team.size());
  // return last for empty array
  if (first == last) {
    return last;
  }
  // Find the local min. element in parallel
  // Get local address range between global iterators:
  auto local_range     = dash::local_subrange(first, last);
  ElementType * lbegin = local_range.begin;
  ElementType * lend   = local_range.end;
  ElementType *lmin    = ::std::min_element(lbegin, lend);     
  if (lmin == lend) {
    // local range is empty
    minarr[team.myid()] = nullptr;
  } else {
    minarr[team.myid()] = first.globmem().globptr(team.myid(), lmin);
  }
  dash::barrier();
  dash::Shared<globptr_t> min; 
  // find the global min. element
  if (team.myid() == 0) {
    globptr_t minloc   = minarr[0];
    ElementType minval = *minloc;
    for (auto i = 1; i < minarr.size(); ++i) {
      if ((globptr_t)minarr[i] != nullptr) {
        ElementType val = *(globptr_t)minarr[i];
        if (val < minval) {
          minloc = minarr[i];
          DASH_LOG_TRACE("Array.min_element", 
                         "Setting min val to ", val);
          minval = val;
        }
      }
    }
    min.set(minloc);
  }
  team.barrier();
  // Minimum has been set by unit 0 at this point
  globptr_t minimum = min.get();
  if (minimum == nullptr) {
    return last;
  }
  return minimum;
}

} // namespace dash

#endif // DASH__ALGORITHM_H_

#ifndef DASH__ALGORITHM__MIN_MAX_H__
#define DASH__ALGORITHM__MIN_MAX_H__

#include <dash/internal/Config.h>

#include <dash/Array.h>
#include <dash/Allocator.h>

#include <dash/algorithm/LocalRange.h>

#include <dash/util/Config.h>
#include <dash/util/Trace.h>
#include <dash/util/UnitLocality.h>

#include <dash/iterator/GlobIter.h>
#include <dash/internal/Logging.h>

#include <algorithm>
#include <memory>

#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif


namespace dash {

/**
 * Finds an iterator pointing to the element with the smallest value in
 * the range [first,last).
 * Specialization for local range, delegates to std::min_element.
 *
 * \return      An iterator to the first occurrence of the smallest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<typename ElementType>
const ElementType * min_element(
  /// Iterator to the initial position in the sequence
  const ElementType * l_range_begin,
  /// Iterator to the final position in the sequence
  const ElementType * l_range_end,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::less<const ElementType &>())
{
#ifdef DASH_ENABLE_OPENMP
  dash::util::UnitLocality uloc;
  auto n_threads = uloc.num_domain_threads();
  DASH_LOG_DEBUG("dash::min_element", "thread capacity:",  n_threads);

  // TODO: Should also restrict on elements/units > ~10240.
  //       Find a model for the minimum work laod.
  if (n_threads > 1) {
    auto          l_size     = l_range_end - l_range_begin;
    int           min_idx_l  = 0;
    ElementType   min_val_l  = *l_range_begin;

    typedef struct min_pos_t { ElementType val; size_t idx; } min_pos;

    DASH_LOG_DEBUG("dash::min_element", "local range size:", l_size);
    int       align_bytes      = uloc.cache_line_size(0);
    size_t    min_vals_t_size  = n_threads + 1 +
                                 (align_bytes / sizeof(min_pos));
    size_t    min_vals_t_bytes = min_vals_t_size * sizeof(min_pos);
    min_pos * min_vals_t_raw   = new min_pos[min_vals_t_size];
    void    * min_vals_t_alg   = min_vals_t_raw;
    min_pos * min_vals_t       = static_cast<min_pos *>(
                                   dash::align(
                                     align_bytes,
                                     sizeof(min_pos),
                                     min_vals_t_alg,
                                     min_vals_t_bytes));
    DASH_LOG_TRACE("dash::min_element", "min * alloc:",   min_vals_t_raw);
    DASH_LOG_TRACE("dash::min_element", "min * aligned:", min_vals_t);
    DASH_LOG_TRACE("dash::min_element", "min * size:",    min_vals_t_bytes);
    DASH_ASSERT_GE(min_vals_t_bytes, n_threads * sizeof(min_pos),
                   "Aligned buffer of min_pos has insufficient size");
    DASH_ASSERT_MSG(nullptr != min_vals_t,
                    "Aligned allocation of min_pos returned nullptr");

    // Cannot use user-defined reduction (OpenMP 4.0) as the compare
    // predicate cannot be used in `omp declare reduction`.
    // Avoid omp for + omp critical section by using array of
    // thread-local minimum values, aligned to prevent false sharing:
    int t_id;
    #pragma omp parallel num_threads(n_threads) private(t_id)
    {
      // Documentation of Intel MIC intrinsics, see:
      // https://software.intel.com/de-de/node/523533
      // https://software.intel.com/de-de/node/523387
      t_id = omp_get_thread_num();
      DASH_LOG_TRACE("dash::min_element", "starting thread", t_id);
      min_vals_t[t_id].idx = min_idx_l;
      min_vals_t[t_id].val = min_val_l;
      // Cannot use explicit private(min_val_t) as ElementType might
      // not be default-constructible:
      #pragma omp for schedule(static)
//    #pragma ivdep
//    #pragma vector aligned nontemporal
      for (int i = 0; i < l_size; i++) {
        const ElementType & val_t = *(l_range_begin + i);
        if (compare(val_t, min_vals_t[t_id].val)) {
          min_vals_t[t_id].val = val_t;
          min_vals_t[t_id].idx = i;
        }
      }
      DASH_LOG_TRACE("dash::min_element", "local minimum at thread", t_id,
                     "idx:", min_vals_t[t_id].idx,
                     "val:", min_vals_t[t_id].val);
    }
    min_pos min_pos_l = min_vals_t[0];
    for (int t = 1; t < n_threads; t++) {
      const min_pos & mpt = min_vals_t[t];
      if (compare(mpt.val, min_pos_l.val)) {
        min_pos_l = mpt;
      }
    }
    delete[] min_vals_t_raw;
    return (l_range_begin + min_pos_l.idx);
  }
#endif // DASH_ENABLE_OPENMP
  return ::std::min_element(l_range_begin, l_range_end, compare);
}

/**
 * Finds an iterator pointing to the element with the smallest value in
 * the range [first,last).
 *
 * \return      An iterator to the first occurrence of the smallest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> min_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::less<const ElementType &>())
{
  typedef dash::GlobIter<ElementType, PatternType> globiter_t;
  typedef PatternType                               pattern_t;
  typedef typename pattern_t::index_type              index_t;

  // return last for empty array
  if (first == last) {
    DASH_LOG_DEBUG("dash::min_element >",
                   "empty range, returning last", last);
    return last;
  }

  dash::util::Trace trace("min_element");

  auto & pattern = first.pattern();
  auto & team    = pattern.team();
  DASH_LOG_DEBUG("dash::min_element()",
                 "allocate minarr, size", team.size());
  // Global position of end element in range:
  auto    gi_last            = last.gpos();
  // Find the local min. element in parallel
  // Get local address range between global iterators:
  auto    local_idx_range    = dash::local_index_range(first, last);
  // Pointer to local minimum element:
  const   ElementType * lmin = nullptr;
  // Local offset of local minimum element, or -1 if no element found:
  index_t l_idx_lmin         = -1;
  if (local_idx_range.begin == local_idx_range.end) {
    // local range is empty
    DASH_LOG_DEBUG("dash::min_element", "local range empty");
  } else {
    trace.enter_state("local");

    // Pointer to first element in local memory:
    const ElementType * lbegin        = first.globmem().lbegin(
                                          dash::Team::GlobalUnitID());
    // Pointers to first / final element in local range:
    const ElementType * l_range_begin = lbegin + local_idx_range.begin;
    const ElementType * l_range_end   = lbegin + local_idx_range.end;

    lmin = dash::min_element(l_range_begin, l_range_end, compare);

    if (lmin != l_range_end) {
      DASH_LOG_TRACE_VAR("dash::min_element", *lmin);
      // Offset of local minimum in local memory:
      l_idx_lmin = lmin - lbegin;
    }

    trace.exit_state("local");
  }
  DASH_LOG_TRACE("dash::min_element",
                 "local index of local minimum:", l_idx_lmin);
  DASH_LOG_TRACE("dash::min_element",
                 "waiting for local min of other units");

  trace.enter_state("barrier");
  team.barrier();
  trace.exit_state("barrier");

  typedef struct {
    ElementType value;
    index_t     g_index;
  } local_min_t;

  std::vector<local_min_t> local_min_values(team.size());

  // Set global index of local minimum to -1 if no local minimum has been
  // found:
  local_min_t local_min;
  local_min.value   = l_idx_lmin < 0
                      ? ElementType()
                      : *lmin;
  local_min.g_index = l_idx_lmin < 0
                      ? -1
                      : pattern.global(l_idx_lmin);

  DASH_LOG_TRACE("dash::min_element", "sending local minimum: {",
                 "value:",   local_min.value,
                 "g.index:", local_min.g_index, "}");

  DASH_LOG_TRACE("dash::min_element", "dart_allgather()");
  trace.enter_state("allgather");
  DASH_ASSERT_RETURNS(
    dart_allgather(
      &local_min,
      local_min_values.data(),
      sizeof(local_min_t),
      DART_TYPE_BYTE,
      team.dart_id()),
    DART_OK);
  trace.exit_state("allgather");

#ifdef DASH_ENABLE_LOGGING
  for (int lmin_u = 0; lmin_u < local_min_values.size(); lmin_u++) {
    auto lmin_entry = local_min_values[lmin_u];
    DASH_LOG_TRACE("dash::min_element", "dart_allgather >",
                   "unit:",    lmin_u,
                   "value:",   lmin_entry.value,
                   "g_index:", lmin_entry.g_index);
  }
#endif

  auto gmin_elem_it  = ::std::min_element(
                           local_min_values.begin(),
                           local_min_values.end(),
                           [&](const local_min_t & a,
                               const local_min_t & b) {
                             // Ignore elements with global index -1 (no
                             // element found):
                             return (b.g_index < 0 ||
                                     (a.g_index > 0 &&
                                      compare(a.value, b.value)));
                           });

  if (gmin_elem_it == local_min_values.end()) {
    DASH_LOG_DEBUG_VAR("dash::min_element >", last);
    return last;
  }

  auto gi_minimum    = gmin_elem_it->g_index;

  DASH_LOG_TRACE("dash::min_element",
                 "min. value:", gmin_elem_it->value,
                 "at unit:",    (gmin_elem_it - local_min_values.begin()),
                 "global idx:", gi_minimum);

  DASH_LOG_TRACE_VAR("dash::min_element", gi_minimum);
  if (gi_minimum < 0 || gi_minimum == gi_last) {
    DASH_LOG_DEBUG_VAR("dash::min_element >", last);
    return last;
  }
  // iterator 'first' is relative to start of input range, convert to start
  // of its referenced container (= container.begin()), then apply global
  // offset of minimum element:
  globiter_t minimum = (first - first.gpos()) + gi_minimum;
  DASH_LOG_DEBUG("dash::min_element >", minimum,
                 "=", static_cast<ElementType>(*minimum));

  return minimum;
}

/**
 * Finds an iterator pointing to the element with the greatest value in
 * the range [first,last).
 *
 * \return      An iterator to the first occurrence of the greatest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template<
  typename ElementType,
  class    PatternType>
GlobIter<ElementType, PatternType> max_element(
  /// Iterator to the initial position in the sequence
  const GlobIter<ElementType, PatternType> & first,
  /// Iterator to the final position in the sequence
  const GlobIter<ElementType, PatternType> & last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::greater<const ElementType &>())
{
  // Same as min_element with different compare function
  return dash::min_element(first, last, compare);
}

/**
 * Finds an iterator pointing to the element with the greatest value in
 * the range [first,last).
 * Specialization for local range, delegates to std::min_element.
 *
 * \return      An iterator to the first occurrence of the greatest value
 *              in the range, or \c last if the range is empty.
 *
 * \tparam      ElementType  Type of the elements in the sequence
 * \complexity  O(d) + O(nl), with \c d dimensions in the global iterators'
 *              pattern and \c nl local elements within the global range
 *
 * \ingroup     DashAlgorithms
 */
template< typename ElementType >
const ElementType * max_element(
  /// Iterator to the initial position in the sequence
  const ElementType * first,
  /// Iterator to the final position in the sequence
  const ElementType * last,
  /// Element comparison function, defaults to std::less
  const std::function<
          bool(const ElementType &, const ElementType)
        > & compare
        = std::greater<const ElementType &>())
{
  // Same as min_element with different compare function
  return dash::min_element(first, last, compare);
}

} // namespace dash

#endif // DASH__ALGORITHM__MIN_MAX_H__

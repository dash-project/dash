#ifndef DASH__ALGORITHM__COPY_H__
#define DASH__ALGORITHM__COPY_H__

#include <dash/GlobIter.h>
#include <dash/algorithm/LocalRange.h>
#include <dash/dart/if/dart_communication.h>
#include <algorithm>

namespace dash {

/**
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * In terms of data distribution, ranges passed to \c dash::copy can be local
 * (\c *ValueType) or global (\c GlobIter<ValueType>).
 *
 * The operation performed is non-blocking if the output iterator is an
 * instance of a \c GlobAsyncIter type.
 * 
 * Example:
 *
 * \code
 *     // Start asynchronous copying
 *     GlobAsyncIter<T> dest_last =
 *       dash::copy(array_a.lbegin(),
 *                  array_a.lend(),
 *                  array_b.async[200]);
 *     // Overlapping computation here
 *     // ...
 *     // Wait for completion of asynchronous copying:
 *     dest_last.fence(); 
 * \endcode
 *
 */
template <
  typename ValueType,
  class InputIt,
  class OutputIt >
OutputIt copy(
  InputIt  in_first,
  InputIt  in_last,
  OutputIt out_first);

namespace internal {

/**
 * Blocking implementation of \c dash::copy (global to local) without
 * optimization for local subrange.
 */
template <
  typename ValueType,
  class GlobInputIt >
ValueType * copy_impl(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType   * out_first)
{
  auto num_elements = dash::distance(in_first, in_last);
  auto num_bytes    = num_elements * sizeof(ValueType);
  DASH_ASSERT_RETURNS(
    dart_get_blocking(
      out_first,
      in_first.dart_gptr(),
      num_bytes),
    DART_OK);
  return out_first + num_elements;
}

/**
 * Blocking implementation of \c dash::copy (local to global) without
 * optimization for local subrange.
 */
template <
  typename ValueType,
  class GlobOutputIt >
GlobOutputIt copy_impl(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  auto num_elements = std::distance(in_first, in_last);
  auto num_bytes    = num_elements * sizeof(ValueType);
  DASH_ASSERT_RETURNS(
    dart_put_blocking(
      out_first.dart_gptr(),
      in_first,
      num_bytes),
    DART_OK);
  return out_first + num_elements;
}

} // namespace internal

/**
 * Specialization of \c dash::copy as global-to-local blocking copy operation.
 */
template <
  typename ValueType,
  class GlobInputIt >
ValueType * copy(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType   * out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to local");
  // Return value, initialize with begin of output range, indicating no values
  // have been copied:
  ValueType * out_last = out_first;
  // Check if part of the input range is local:
  auto li_range_in = local_index_range(in_first, in_last);
  DASH_LOG_TRACE_VAR("dash::copy", li_range_in.begin);
  DASH_LOG_TRACE_VAR("dash::copy", li_range_in.end);
  if (li_range_in.begin != li_range_in.end) {
    // Part of the input range is local.
    // Copy local input subrange to local output range directly:
    auto pattern           = in_first.pattern();
    // Number of elements in the local subrange:
    auto l_range_size      = li_range_in.end - li_range_in.begin;
    DASH_LOG_TRACE("dash::copy", "resolving local subrange");
    DASH_LOG_TRACE_VAR("dash::copy", l_range_size);
    // Local index range to global input index range:
    auto l_g_offset_begin  = pattern.local_to_global_index(li_range_in.begin);
    DASH_LOG_TRACE_VAR("dash::copy", l_g_offset_begin);
    auto l_g_offset_end    = pattern.local_to_global_index(li_range_in.end-1)
                             + 1; // pat.global(l_end) would be out of range
    DASH_LOG_TRACE_VAR("dash::copy", l_g_offset_end);
    // Convert local subrange of global input to native pointers:
    ValueType * l_in_first = ((GlobPtr<ValueType>)
                              (in_first + l_g_offset_begin));
    DASH_LOG_TRACE_VAR("dash::copy", l_in_first);
    ValueType * l_in_last  = ((GlobPtr<ValueType>)
                              (in_first + (l_g_offset_end - 1))) + 1;
    DASH_LOG_TRACE_VAR("dash::copy", l_in_last);
    // ... [ ........ | --- l --- | ........ ]
    //     ^          ^           ^          ^
    //     in_first   l_in_first  l_in_last  in_last
    DASH_LOG_TRACE("dash::copy", "copying local subrange");
    out_last = std::copy(l_in_first,
                         l_in_last,
                         out_first + l_g_offset_begin);
    // Assert that all elements in local range have been copied:
    DASH_ASSERT(out_last == out_first + l_g_offset_end);
    // Copy remote elements preceding the local subrange:
    if (l_g_offset_begin > in_first.pos()) {
      DASH_LOG_TRACE("dash::copy", "copy global preceding local subrange");
      // ... [ --- copy --- | ... l ... | ........ ]
      //     ^              ^           ^          ^
      //     in_first       l_in_first  l_in_last  in_last
      out_last = dash::internal::copy_impl(in_first,
                                           in_first + l_g_offset_begin,
                                           out_first);
    }
    // Copy remote elements succeeding the local subrange:
    if (l_g_offset_end < in_last.pos()) {
      DASH_LOG_TRACE("dash::copy", "copy global succeeding local subrange");
      // ... [ ........ | ... l ... | --- copy --- ]
      //     ^          ^           ^              ^
      //     in_first   l_in_first  l_in_last      in_last
      out_last = dash::internal::copy_impl(in_first + l_g_offset_end,
                                           in_last,
                                           out_first + l_range_size);
    }
  } else {
    DASH_LOG_TRACE("dash::copy", "no local subrange");
    // All elements in input range are remote
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first);
  }
  return out_last;
}

/**
 * Specialization of \c dash::copy as local-to-global blocking copy operation.
 */
template <
  typename ValueType,
  class GlobOutputIt >
GlobOutputIt copy(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, local to global");
  // Return value, initialize with begin of output range, indicating no values
  // have been copied:
  GlobOutputIt out_last   = out_first;
  // Number of elements to copy in total:
  auto num_elements       = std::distance(in_first, in_last);
  DASH_LOG_TRACE_VAR("dash::copy", num_elements);
  // Global iterator pointing at hypothetical end of output range:
  GlobOutputIt out_h_last = out_first + num_elements;
  DASH_LOG_TRACE_VAR("dash::copy", out_first.pos());
  DASH_LOG_TRACE_VAR("dash::copy", out_h_last.pos());
  // Test if subrange of global output range is local:
  auto li_range_out       = local_index_range(out_first, out_h_last);
  DASH_LOG_TRACE_VAR("dash::copy", li_range_out.begin);
  DASH_LOG_TRACE_VAR("dash::copy", li_range_out.end);
  // Check if part of the output range is local:
  if (li_range_out.begin != li_range_out.end) {
    // Part of the output range is local
    // Copy local input subrange to local output range directly:
    auto pattern            = out_first.pattern();
    // Number of elements in the local subrange:
    auto l_range_size       = li_range_out.end - li_range_out.begin;
    DASH_LOG_TRACE("dash::copy", "resolving local subrange");
    DASH_LOG_TRACE_VAR("dash::copy", l_range_size);
    // Local index range to global output index range:
    auto l_g_offset_begin   = pattern.local_to_global_index(li_range_out.begin);
    DASH_LOG_TRACE_VAR("dash::copy", l_g_offset_begin);
    auto l_g_offset_end     = pattern.local_to_global_index(li_range_out.end-1)
                              + 1; // pat.global(l_end) would be out of range
    DASH_LOG_TRACE_VAR("dash::copy", l_g_offset_end);
    // Offset of local subrange in output range
    auto l_elem_offset      = l_g_offset_begin - out_first.pos();
    DASH_LOG_TRACE_VAR("dash::copy",l_elem_offset);
    // Convert local subrange of global output to native pointers:
    ValueType * l_out_first = ((GlobPtr<ValueType>)
                               (out_first + l_elem_offset));
    DASH_LOG_TRACE_VAR("dash::copy", l_out_first);
    ValueType * l_out_last  = l_out_first + l_range_size;
    DASH_LOG_TRACE_VAR("dash::copy", l_out_last);
    // ... [ ........ | ---- l ---- | ......... ] ...
    //     ^          ^             ^           ^
    //     out_first  l_out_first   l_out_last  out_last
    out_last                = out_first + l_range_size;
    // Assert that all elements in local range have been copied:
    DASH_LOG_TRACE("dash::copy", "copying local subrange");
    DASH_LOG_TRACE_VAR("dash::copy", in_first);
    DASH_ASSERT(
      std::copy(in_first + l_elem_offset,
                in_first + l_elem_offset + l_range_size,
                l_out_first) == l_out_last);
    // Copy to remote elements preceding the local subrange:
    if (l_g_offset_begin > out_first.pos()) {
      DASH_LOG_TRACE("dash::copy", "copy to global preceding local subrange");
      out_last = dash::internal::copy_impl(in_first,
                                           in_first + l_elem_offset,
                                           out_first);
    }
    // Copy to remote elements succeeding the local subrange:
    if (l_g_offset_end < out_h_last.pos()) {
      DASH_LOG_TRACE("dash::copy", "copy to global succeeding local subrange");
      out_last = dash::internal::copy_impl(in_first + l_elem_offset + l_range_size,
                                           in_last,
                                           out_first + l_range_size);
    }
  } else {
    // All elements in output range are remote
    DASH_LOG_TRACE("dash::copy", "no local subrange");
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first);
  }
  return out_last;
}

} // namespace dash

#endif // DASH__ALGORITHM__COPY_H__

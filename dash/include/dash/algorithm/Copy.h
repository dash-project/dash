#ifndef DASH__ALGORITHM__COPY_H__
#define DASH__ALGORITHM__COPY_H__

#include <dash/Future.h>
#include <dash/Iterator.h>

#include <dash/View.h>
#include <dash/view/IndexSet.h>

#include <dash/Range.h>

#include <dash/view/IndexSet.h>
#include <dash/view/ViewMod.h>
#include <dash/view/ViewTraits.h>
#include <dash/view/ViewBlocksMod.h>
#include <dash/view/Sub.h>

#include <dash/algorithm/LocalRange.h>

#include <dash/dart/if/dart_communication.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <future>


namespace dash {

#ifdef DOXYGEN

/**
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::copy can be local (\c *ValueType) or global
 * (\c GlobIter<ValueType>).
 *
 * For a non-blocking variant of \c dash::copy, see \c dash::copy_async.
 *
 * Example:
 *
 * \code
 *     // Start blocking copy
 *     auto copy_last =
 *       dash::copy(array_a.lbegin(),
 *                  array_a.lend(),
 *                  array_b.begin() + 200);
 *     auto ncopied = dash::distance(array_b.begin() + 200, copy_last);
 * \endcode
 *
 * \returns  The output range end iterator that is created on completion
 *           of the copy operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class InputIt,
  class OutputIt >
OutputIt copy(
  const InputIt in_first,
  const InputIt in_last,
  OutputIt      out_first);

/**
 * Asynchronous variant of \c dash::copy.
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::copy can be local (\c *ValueType) or global
 * (\c GlobIter<ValueType>).
 *
 * For a blocking variant of \c dash::copy_async, see \c dash::copy.
 *
 * Example:
 *
 * \code
 *     // Start asynchronous copying
 *     dash::Future<T*> fut_dest_end =
 *       dash::copy_async(array_a.block(0).begin(),
 *                        array_a.block(0).end(),
 *                        local_array);
 *     // Overlapping computation here
 *     // ...
 *     // Wait for completion of asynchronous copying:
 *     T * copy_dest_end = fut_dest_end.get();
 * \endcode
 *
 * \returns  An instance of \c dash::Future providing the output range end
 *           iterator that is created on completion of the asynchronous 
 *           copy operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class GlobInputIt >
dash::Future<ValueType *> copy_async(
  const InputIt  in_first,
  const InputIt  in_last,
  OutputIt       out_first);

#else // DOXYGEN

namespace internal {

// =======================================================================
// Global to Local
// =======================================================================

/**
 * Blocking implementation of \c dash::copy (global to local) without
 * optimization for local subrange.
 */
template <
  class ValueType,
  class GlobInputIt >
ValueType * copy_block(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType         * out_first)
{
  DASH_LOG_TRACE("dash::copy_block()",
                 "in_first:",  in_first.pos(),
                 "in_last:",   in_last.pos(),
                 "out_first:", out_first);
  auto pattern = in_first.pattern();
  typedef typename decltype(pattern)::index_type index_type;
  typedef typename decltype(pattern)::size_type  size_type;
  size_type num_elem_total = dash::distance(in_first, in_last);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy_block", "input range empty");
    return out_first;
  }
  DASH_LOG_TRACE("dash::copy_block",
                 "total elements:",    num_elem_total,
                 "expected out_last:", out_first + num_elem_total);
  // Input iterators could be relative to a view. Map first input iterator
  // to global index range and use it to resolve last input iterator.
  // Do not use in_last.global() as this would span over the relative
  // input range.
  auto g_in_first      = in_first.global();
  auto g_in_last       = g_in_first + num_elem_total;
  DASH_LOG_TRACE("dash::copy_block",
                 "g_in_first:", g_in_first.pos(),
                 "g_in_last:",  g_in_last.pos());
  auto unit_first      = pattern.unit_at(g_in_first.pos());
  DASH_LOG_TRACE_VAR("dash::copy_block", unit_first);
  auto unit_last       = pattern.unit_at(g_in_last.pos() - 1);
  DASH_LOG_TRACE_VAR("dash::copy_block", unit_last);

  // MPI uses offset type int, do not copy more than INT_MAX bytes:
  size_type max_copy_elem   = (std::numeric_limits<int>::max() /
                               sizeof(ValueType));
  size_type num_elem_copied = 0;
  DASH_LOG_TRACE_VAR("dash::copy_block", max_copy_elem);
  if (num_elem_total > max_copy_elem) {
    DASH_LOG_DEBUG("dash::copy_block",
                   "cannot copy", num_elem_total, "elements",
                   "in a single dart_get operation");
  }
  if (unit_first == unit_last) {
    // Input range is located at a single remote unit:
    DASH_LOG_TRACE("dash::copy_block", "input range at single unit");
    while (num_elem_copied < num_elem_total) {
      // Number of elements left to copy:
      auto total_elem_left = num_elem_total - num_elem_copied;
      auto num_copy_elem   = (num_elem_total > max_copy_elem)
                             ? max_copy_elem
                             : num_elem_total;
      if (num_copy_elem > total_elem_left) {
        num_copy_elem = total_elem_left;
      }
      DASH_LOG_TRACE("dash::copy_block",
                     "copy max:",       max_copy_elem,
                     "get elements:",   num_copy_elem,
                     "total:",          num_elem_total,
                     "copied:",         num_elem_copied,
                     "left:",           total_elem_left);
      auto cur_in_first  = g_in_first + num_elem_copied;
      auto cur_out_first = out_first  + num_elem_copied;
      dart_storage_t ds = dash::dart_storage<ValueType>(num_copy_elem);
      DASH_ASSERT_RETURNS(
        dart_get_blocking(
          cur_out_first,
          cur_in_first.dart_gptr(),
          ds.nelem,
          ds.dtype),
        DART_OK);
      num_elem_copied += num_copy_elem;
    }
  } else {
    // Input range is spread over several remote units:
    DASH_LOG_TRACE("dash::copy_block",
                   "input range spans multiple units");
    //
    // Copy elements from every unit:
    //
    while (num_elem_copied < num_elem_total) {
      // Global iterator pointing at begin of current unit's input range:
      auto cur_in_first    = g_in_first + num_elem_copied;
      // unit and local index of first element in current range segment:
      auto local_pos       = pattern.local(static_cast<index_type>(
                                             cur_in_first.pos()));
      // Number of elements located at current source unit:
      size_type max_elem_per_unit = pattern.local_size(local_pos.unit);
      // Local offset of first element in input range at current unit:
      auto l_in_first_idx  = local_pos.index;
      // Maximum number of elements to copy from current unit:
      auto num_unit_elem   = max_elem_per_unit - l_in_first_idx;
      // Number of elements left to copy:
      auto total_elem_left = num_elem_total - num_elem_copied;
      // Number of elements to copy in this iteration.
      auto num_copy_elem   = (num_unit_elem < max_copy_elem)
                             ? num_unit_elem
                             : max_copy_elem;
      if (num_copy_elem > total_elem_left) {
        num_copy_elem = total_elem_left;
      }
      DASH_ASSERT_GT(num_copy_elem, 0,
                     "Number of element to copy is 0");
      DASH_LOG_TRACE("dash::copy_block",
                     "start g_idx:",    cur_in_first.pos(),
                     "->",
                     "unit:",           local_pos.unit,
                     "l_idx:",          l_in_first_idx,
                     "->",
                     "unit elements:",  num_unit_elem,
                     "max elem/unit:",  max_elem_per_unit,
                     "copy max:",       max_copy_elem,
                     "get elements:",   num_copy_elem,
                     "total:",          num_elem_total,
                     "copied:",         num_elem_copied,
                     "left:",           total_elem_left);
      auto dest_ptr = out_first + num_elem_copied;
      auto src_gptr = cur_in_first.dart_gptr();
      dart_storage_t ds = dash::dart_storage<ValueType>(num_copy_elem);
      if (dart_get_blocking(
            dest_ptr,
            src_gptr,
            ds.nelem,
            ds.dtype)
          != DART_OK) {
        DASH_LOG_ERROR("dash::copy_block", "dart_get failed");
        DASH_THROW(
          dash::exception::RuntimeError, "dart_get failed");
      }
      num_elem_copied += num_copy_elem;
    }
  }

  ValueType * out_last = out_first + num_elem_copied;
  DASH_LOG_TRACE_VAR("dash::copy_block >", out_last);
  return out_last;
}

/**
 * Asynchronous implementation of \c dash::copy (global to local) without
 * optimization for local subrange.
 */
template <
  class ValueType,
  class GlobInputIt >
dash::Future<ValueType *> copy_block_async(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType         * out_first)
{
  DASH_LOG_TRACE("dash::copy_block_async()",
                 "in_first:",  in_first.pos(),
                 "in_last:",   in_last.pos(),
                 "out_first:", out_first);
  auto pattern = in_first.pattern();
  typedef typename decltype(pattern)::index_type index_type;
  typedef typename decltype(pattern)::size_type  size_type;
  size_type num_elem_total = dash::distance(in_first, in_last);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy_block_async", "input range empty");
    return dash::Future<ValueType *>([=]() { return out_first; });
  }
  DASH_LOG_TRACE("dash::copy_block_async",
                 "total elements:",    num_elem_total,
                 "expected out_last:", out_first + num_elem_total);
  // Input iterators could be relative to a view. Map first input iterator
  // to global index range and use it to resolve last input iterator.
  // Do not use in_last.global() as this would span over the relative 
  // input range.
  auto g_in_first      = in_first.global();
  auto g_in_last       = g_in_first + num_elem_total;
  DASH_LOG_TRACE("dash::copy_block_async",
                 "g_in_first:", g_in_first.pos(),
                 "g_in_last:",  g_in_last.pos());
  auto unit_first      = pattern.unit_at(g_in_first.pos());
  DASH_LOG_TRACE_VAR("dash::copy_block_async", unit_first);
  auto unit_last       = pattern.unit_at(g_in_last.pos() - 1);
  DASH_LOG_TRACE_VAR("dash::copy_block_async", unit_last);

  // Accessed global pointers to be flushed:
  std::vector<dart_handle_t> req_handles;

  // MPI uses offset type int, do not copy more than INT_MAX bytes:
  size_type max_copy_elem   = (std::numeric_limits<int>::max() /
                               sizeof(ValueType));
  size_type num_elem_copied = 0;
  DASH_LOG_TRACE_VAR("dash::copy_block_async", max_copy_elem);
  if (num_elem_total > max_copy_elem) {
    DASH_LOG_DEBUG("dash::copy_block_async",
                   "cannot copy", num_elem_total, "elements",
                   "in a single dart_get operation");
  }
  if (unit_first == unit_last) {
    // Input range is located at a single remote unit:
    DASH_LOG_TRACE("dash::copy_block_async",
                   "input range at single unit");
    while (num_elem_copied < num_elem_total) {
      // Number of elements left to copy:
      auto total_elem_left = num_elem_total - num_elem_copied;
      auto num_copy_elem   = (num_elem_total > max_copy_elem)
                             ? max_copy_elem
                             : num_elem_total;
      if (num_copy_elem > total_elem_left) {
        num_copy_elem = total_elem_left;
      }
      DASH_LOG_TRACE("dash::copy_block_async",
                     "copy max:",       max_copy_elem,
                     "get elements:",   num_copy_elem,
                     "total:",          num_elem_total,
                     "copied:",         num_elem_copied,
                     "left:",           total_elem_left);
      auto cur_in_first  = g_in_first + num_elem_copied;
      auto cur_out_first = out_first  + num_elem_copied;
      dart_handle_t  get_handle;
      dart_storage_t ds = dash::dart_storage<ValueType>(num_copy_elem);
      DASH_ASSERT_RETURNS(
        dart_get_handle(
          cur_out_first,
          cur_in_first.dart_gptr(),
          ds.nelem,
          ds.dtype,
          &get_handle),
        DART_OK);
      if (get_handle != NULL) {
        req_handles.push_back(get_handle);
      }
      num_elem_copied += num_copy_elem;
    }
  } else {
    // Input range is spread over several remote units:
    DASH_LOG_TRACE("dash::copy_block_async",
                   "input range spans multiple units");
    //
    // Copy elements from every unit:
    //
    while (num_elem_copied < num_elem_total) {
      // Global iterator pointing at begin of current unit's input range:
      auto cur_in_first    = g_in_first + num_elem_copied;
      // unit and local index of first element in current range segment:
      auto local_pos       = pattern.local(static_cast<index_type>(
                                             cur_in_first.pos()));
      // Number of elements located at current source unit:
      size_type max_elem_per_unit = pattern.local_size(local_pos.unit);
      // Local offset of first element in input range at current unit:
      auto l_in_first_idx  = local_pos.index;
      // Maximum number of elements to copy from current unit:
      auto num_unit_elem   = max_elem_per_unit - l_in_first_idx;
      // Number of elements left to copy:
      auto total_elem_left = num_elem_total - num_elem_copied;
      // Number of elements to copy in this iteration.
      auto num_copy_elem   = (num_unit_elem < max_copy_elem)
                             ? num_unit_elem
                             : max_copy_elem;
      if (num_copy_elem > total_elem_left) {
        num_copy_elem = total_elem_left;
      }
      DASH_ASSERT_GT(num_copy_elem, 0,
                     "Number of element to copy is 0");
      DASH_LOG_TRACE("dash::copy_block_async",
                     "start g_idx:",    cur_in_first.pos(),
                     "->",
                     "unit:",           local_pos.unit,
                     "l_idx:",          l_in_first_idx,
                     "->",
                     "unit elements:",  num_unit_elem,
                     "max elem/unit:",  max_elem_per_unit,
                     "copy max:",       max_copy_elem,
                     "get elements:",   num_copy_elem,
                     "total:",          num_elem_total,
                     "copied:",         num_elem_copied,
                     "left:",           total_elem_left);
      auto src_gptr = cur_in_first.dart_gptr();
      auto dest_ptr = out_first + num_elem_copied;
      dart_handle_t  get_handle;
      dart_storage_t ds = dash::dart_storage<ValueType>(num_copy_elem);
      DASH_ASSERT_RETURNS(
        dart_get_handle(
          dest_ptr,
          src_gptr,
          ds.nelem,
          ds.dtype,
          &get_handle),
        DART_OK);
      if (get_handle != NULL) {
        req_handles.push_back(get_handle);
      }
      num_elem_copied += num_copy_elem;
    }
  }
#ifdef DASH_ENABLE_TRACE_LOGGING
  for (auto gptr : req_handles) {
    DASH_LOG_TRACE("dash::copy_block_async", "  req_handle:", gptr);
  }
#endif
  dash::Future<ValueType *> result([=]() mutable {
    // Wait for all get requests to complete:
    ValueType * _out = out_first + num_elem_copied;
    DASH_LOG_TRACE("dash::copy_block_async [Future]()",
                   "  wait for", req_handles.size(), "async get request");
    DASH_LOG_TRACE("dash::copy_block_async [Future]", "  flush:",
                   req_handles);
#ifdef DASH_ENABLE_TRACE_LOGGING
    for (auto gptr : req_handles) {
      DASH_LOG_TRACE("dash::copy_block_async [Future]", "  req_handle:",
                     gptr);
    }
#endif
    if (req_handles.size() > 0) {
      if (dart_waitall_local(&req_handles[0], req_handles.size())
          != DART_OK) {
        DASH_LOG_ERROR("dash::copy_block_async [Future]",
                       "  dart_waitall_local failed");
        DASH_THROW(
          dash::exception::RuntimeError,
          "dash::copy_block_async [Future]: dart_waitall_local failed");
      }
    } else {
      DASH_LOG_TRACE("dash::copy_block_async [Future]",
                     "  No pending handles");
    }
    DASH_LOG_TRACE("dash::copy_block_async [Future] >",
                   "  async requests completed, _out:", _out);
    return _out;
  });
  DASH_LOG_TRACE("dash::copy_block_async >", "  returning future");
  return result;
}

// =======================================================================
// Local to Global
// =======================================================================

/**
 * Blocking implementation of \c dash::copy (local to global) without
 * optimization for local subrange.
 */
template <
  class ValueType,
  class GlobOutputIt >
GlobOutputIt copy_block(
  ValueType * in_first,
  ValueType * in_last,
  GlobOutputIt      out_first)
{
  DASH_LOG_TRACE("dash::copy_block()",
                 "l_in_first:",      in_first,
                 "l_in_last:",       in_last,
                 "g_out_first.pos:", out_first.pos());
  DASH_LOG_TRACE("dash::copy_block",
                 "g_out_first:", out_first, out_first.dart_gptr());

  auto num_elements = std::distance(in_first, in_last);
  dart_storage_t ds = dash::dart_storage<ValueType>(num_elements);
  DASH_ASSERT_RETURNS(
    dart_put_blocking(
      out_first.dart_gptr(),
      in_first,
      ds.nelem,
      ds.dtype),
    DART_OK);

  auto out_last = out_first + num_elements;
  DASH_LOG_TRACE("dash::copy_block >",
                 "g_out_last:", out_last.dart_gptr());

  return out_last;
}

/**
 * Asynchronous implementation of \c dash::copy (local to global) without
 * optimization for local subrange.
 */
template <
  class ValueType,
  class GlobOutputIt >
dash::Future<GlobOutputIt> copy_block_async(
  ValueType * in_first,
  ValueType * in_last,
  GlobOutputIt      out_first)
{
  DASH_LOG_TRACE("dash::copy_block_async()",
                 "l_in_first:",  in_first,
                 "l_in_last:",   in_last,
                 "g_out_first:", out_first.dart_gptr());

  // Accessed global pointers to be flushed:
  std::vector<dart_handle_t> req_handles;

  auto num_copy_elem = std::distance(in_first, in_last);
  auto src_ptr       = in_first;
  auto dest_gptr     = out_first.dart_gptr();
  dart_handle_t  put_handle;
  dart_storage_t ds = dash::dart_storage<ValueType>(num_copy_elem);
  DASH_ASSERT_RETURNS(
    dart_put_handle(
        dest_gptr,
        src_ptr,
        ds.nelem,
        ds.dtype,
        &put_handle),
    DART_OK);
  if (put_handle != NULL) {
    req_handles.push_back(put_handle);
  }

#ifdef DASH_ENABLE_TRACE_LOGGING
  for (auto gptr : req_handles) {
    DASH_LOG_TRACE("dash::copy_block_async", "  req_handle:", gptr);
  }
#endif
  dash::Future<GlobOutputIt> result([=]() mutable {
    // Wait for all get requests to complete:
    GlobOutputIt _out = out_first + num_copy_elem;
    DASH_LOG_TRACE("dash::copy_block_async [Future]()",
                   "  wait for", req_handles.size(), "async put request");
    DASH_LOG_TRACE("dash::copy_block_async [Future]", "  flush:",
                   req_handles);
    DASH_LOG_TRACE("dash::copy_block_async [Future]", "  _out:", _out);
#ifdef DASH_ENABLE_TRACE_LOGGING
    for (auto gptr : req_handles) {
      DASH_LOG_TRACE("dash::copy_block_async [Future]", "  req_handle:",
                     gptr);
    }
#endif
    if (req_handles.size() > 0) {
      if (dart_waitall(&req_handles[0], req_handles.size())
          != DART_OK) {
        DASH_LOG_ERROR("dash::copy_block_async [Future]",
                       "  dart_waitall failed");
        DASH_THROW(
          dash::exception::RuntimeError,
          "dash::copy_block_async [Future]: dart_waitall failed");
      }
    } else {
      DASH_LOG_TRACE("dash::copy_block_async [Future]",
                     "  No pending handles");
    }
    DASH_LOG_TRACE("dash::copy_block_async [Future] >",
                   "  async requests completed, _out:", _out);
    return _out;
  });
  DASH_LOG_TRACE("dash::copy_block_async >", "  returning future");
  return result;
}

} // namespace internal




// =======================================================================
// Global to Local, Distributed Range
// =======================================================================

/**
 * Variant of \c dash::copy as asynchronous global-to-local copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class ValueType,
  class GlobInputIt >
dash::Future<ValueType *> copy_async(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType         * out_first)
{
  const auto & team = in_first.team();
  dash::util::UnitLocality uloc(team, team.myid());
  // Size of L2 data cache line:
  int  l2_line_size = uloc.hwinfo().cache_line_sizes[1];
  bool use_memcpy   = ((in_last - in_first) * sizeof(ValueType))
                      <= l2_line_size;

  DASH_LOG_TRACE("dash::copy_async()", "async, global to local");
  if (in_first == in_last) {
    DASH_LOG_TRACE("dash::copy_async", "input range empty");
    return dash::Future<ValueType *>([=]() { return out_first; });
  }
  ValueType * dest_first = out_first;
  // Return value, initialize with begin of output range, indicating noi
  // values have been copied:
  ValueType * out_last   = out_first;
  // Check if part of the input range is local:
  DASH_LOG_TRACE_VAR("dash::copy_async", in_first.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy_async", in_last.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy_async", out_first);
  auto li_range_in     = local_index_range(in_first, in_last);
  // Number of elements in the local subrange:
  auto num_local_elem  = li_range_in.end - li_range_in.begin;
  // Total number of elements to be copied:
  auto total_copy_elem = in_last - in_first;

  // Instead of testing in_first.local() and in_last.local(), this test 
  // for a local-only range only requires one call to in_first.local()
  // which increases throughput by ~10% for local ranges.
  if (num_local_elem == total_copy_elem) {
    // Entire input range is local:
    DASH_LOG_TRACE("dash::copy_async", "entire input range is local");
    ValueType * l_out_last = out_first + total_copy_elem;
    ValueType * l_in_first = in_first.local();
    ValueType * l_in_last  = l_in_first + total_copy_elem;

    // Use memcpy for data ranges below 64 KB
    if (use_memcpy) {
      std::memcpy(out_first,        // destination
                  in_first.local(), // source
                  num_local_elem * sizeof(ValueType));
      out_last = out_first + num_local_elem;
    } else {
      ValueType * l_in_first = in_first.local();
      ValueType * l_in_last  = l_in_first + num_local_elem;
      out_last = std::copy(l_in_first,
                           l_in_last,
                           out_first);
    }
    DASH_LOG_TRACE("dash::copy_async", "finished local copy of",
                   (out_last - out_first), "elements");
    return dash::Future<ValueType *>([=]() { return out_last; });
  }

  DASH_LOG_TRACE("dash::copy_async", "local range:",
                 li_range_in.begin,
                 li_range_in.end,
                 "in_first.is_local:", in_first.is_local());
  // Futures of asynchronous get requests:
  auto futures = std::vector< dash::Future<ValueType *> >();
  // Check if global input range is partially local:
  if (num_local_elem > 0) {
    // Part of the input range is local, copy local input subrange to
    // local output range directly.
    auto pattern          = in_first.pattern();
    // Map input iterators to global index domain:
    auto g_in_first       = in_first.global();
    auto g_in_last        = g_in_first + total_copy_elem;
    DASH_LOG_TRACE("dash::copy_async", "resolving local subrange");
    DASH_LOG_TRACE_VAR("dash::copy_async", num_local_elem);
    // Local index range to global input index range:
    // Global index of local range begin index:
    auto g_l_offset_begin = pattern.global(li_range_in.begin);
    // Global index of local range end index:
    //
    // NOTE: definition as pat.global(l_end-1)+1 as
    //       pat.global(l_end) would be out of range
    auto g_l_offset_end   = pattern.global(li_range_in.end-1) + 1;
    DASH_LOG_TRACE("dash::copy_async",
                   "global index range of local subrange:",
                   "begin:", g_l_offset_begin, "end:", g_l_offset_end);
    // Global position of input start iterator:
    auto g_offset_begin   = g_in_first.pos();
    // Convert local subrange to global iterators:
    auto g_l_in_first     = g_in_first +
                            (g_l_offset_begin - g_offset_begin);
    auto g_l_in_last      = g_in_first +
                            (g_l_offset_end   - g_offset_begin);
    DASH_LOG_TRACE("dash::copy_async",
                   "global it. range of local subrange:",
                   "begin:", g_l_in_first.pos(),
                   "end:",   g_l_in_last.pos());
    DASH_LOG_TRACE_VAR("dash::copy_async", g_l_in_last.pos());
    //
    // -------------------------------------------------------------------
    // Copy remote elements preceding the local subrange:
    //
    auto num_prelocal_elem = g_l_in_first.pos() - g_in_first.pos();
    DASH_LOG_TRACE_VAR("dash::copy_async", num_prelocal_elem);
    if (num_prelocal_elem > 0) {
      DASH_LOG_TRACE("dash::copy_async",
                     "copy global range preceding local subrange",
                     "g_in_first:", g_in_first.pos(),
                     "g_in_last:",  g_l_in_first.pos());
      // ... [ --- copy --- | ... l ... | ........ ]
      //     ^              ^           ^          ^
      //     in_first       l_in_first  l_in_last  in_last
      auto fut_prelocal = dash::internal::copy_block_async(g_in_first,
                                                          g_l_in_first,
                                                          dest_first);
      futures.push_back(fut_prelocal);
      // Advance output pointers:
      out_last   += num_prelocal_elem;
      dest_first  = out_last;
    }
    //
    // -------------------------------------------------------------------
    // Copy remote elements succeeding the local subrange:
    //
    auto num_postlocal_elem = in_last.pos() - g_l_offset_end;
    DASH_LOG_TRACE_VAR("dash::copy_async", num_postlocal_elem);
    if (num_postlocal_elem > 0) {
      dest_first += num_local_elem;
      DASH_LOG_TRACE("dash::copy_async",
                     "copy global range succeeding local subrange",
                     "in_first:", g_l_in_last.pos(),
                     "in_last:",  g_in_last.pos());
      // ... [ ........ | ... l ... | --- copy --- ]
      //     ^          ^           ^              ^
      //     in_first   l_in_first  l_in_last      in_last
      auto fut_postlocal = dash::internal::copy_block_async(g_l_in_last,
                                                           g_in_last,
                                                           dest_first);
      futures.push_back(fut_postlocal);
      out_last += num_postlocal_elem;
    }
    //
    // -------------------------------------------------------------------
    // Copy local subrange:
    //
    // Convert local subrange of global input to native pointers:
    //
    // ... [ ........ | --- l --- | ........ ]
    //     ^          ^           ^          ^
    //     in_first   l_in_first  l_in_last  in_last
    //
    ValueType * l_in_first = g_l_in_first.local();
    ValueType * l_in_last  = l_in_first + num_local_elem;
    DASH_LOG_TRACE_VAR("dash::copy_async", l_in_first);
    DASH_LOG_TRACE_VAR("dash::copy_async", l_in_last);
    // Verify conversion of global input iterators to local pointers:
    DASH_ASSERT_MSG(l_in_first != nullptr,
                    "dash::copy_async: first index in global input (" <<
                    g_l_in_first.pos() << ") is not local");

    DASH_LOG_TRACE("dash::copy_async", "copy local subrange",
                   "num_copy_elem:", l_in_last - l_in_first);
    ValueType * local_out_first = out_first + num_prelocal_elem;
    ValueType * local_out_last  = local_out_first + num_local_elem;

    // Use memcpy for data ranges below 64 KB
    if (use_memcpy) {
      std::memcpy(local_out_first, // destination
                  l_in_first,      // source
                  num_local_elem * sizeof(ValueType));
      local_out_last = local_out_first + num_local_elem;
    } else {
      local_out_last = std::copy(l_in_first,
                                 l_in_last,
                                 local_out_first);
    }
    DASH_LOG_TRACE("dash::copy_async", "<< std::shared_future >>",
                   "finished local copy of",
                   (local_out_last - local_out_first),
                   "elements");
    out_last += (local_out_last - local_out_first);
  } else {
    DASH_LOG_TRACE("dash::copy_async", "no local subrange");
    // All elements in input range are remote
    auto fut_all = dash::internal::copy_block_async(in_first,
                                                   in_last,
                                                   dest_first);
    futures.push_back(fut_all);
    out_last = out_first + total_copy_elem;
  }
  DASH_LOG_TRACE("dash::copy_async", "preparing future");
  dash::Future<ValueType *> fut_result([=]() mutable {
    ValueType * _out = out_last;
    DASH_LOG_TRACE("dash::copy_async [Future]()",
                   "wait for", futures.size(), "async copy requests");
    DASH_LOG_TRACE("dash::copy_async [Future]", "  futures:", futures);
    DASH_LOG_TRACE("dash::copy_async [Future]", "  _out:", _out);
    for (auto f : futures) {
      f.wait();
    }
    DASH_LOG_TRACE("dash::copy_async [Future] >",
                   "async requests completed",
                   "futures:", futures, "_out:", _out);
    return _out;
  });
  DASH_LOG_TRACE("dash::copy_async >", "finished,",
                 "expected out_last:", out_last);
  return fut_result;
}

/*
 * Specialization of \c dash::copy as global-to-local blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class ValueType,
  class GlobInputIt >
ValueType * copy(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType   * out_first)
{
  const auto & team = in_first.team();
  dash::util::UnitLocality uloc(team, team.myid());
  // Size of L2 data cache line:
  int  l2_line_size = uloc.hwinfo().cache_line_sizes[1];
  bool use_memcpy   = ((in_last - in_first) * sizeof(ValueType))
                      <= l2_line_size;

  DASH_LOG_TRACE("dash::copy()", "blocking, global to local");

  ValueType * dest_first = out_first;
  // Return value, initialize with begin of output range, indicating no
  // values have been copied:
  ValueType * out_last   = out_first;
  // Check if part of the input range is local:
  DASH_LOG_TRACE_VAR("dash::copy", in_first.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", in_last.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", out_first);
  auto li_range_in     = local_index_range(in_first, in_last);
  // Number of elements in the local subrange:
  auto num_local_elem  = li_range_in.end - li_range_in.begin;
  // Total number of elements to be copied:
  auto total_copy_elem = in_last - in_first;

  // Instead of testing in_first.local() and in_last.local(), this test
  // for a local-only range only requires one call to in_first.local()
  // which increases throughput by ~10% for local ranges.
  if (num_local_elem == total_copy_elem) {
    // Entire input range is local:
    DASH_LOG_TRACE("dash::copy", "entire input range is local");
    ValueType * out_last = out_first + total_copy_elem;
    // Use memcpy for data ranges below 64 KB
    if (use_memcpy) {
      std::memcpy(out_first,        // destination
                  in_first.local(), // source
                  num_local_elem * sizeof(ValueType));
      out_last = out_first + num_local_elem;
    } else {
      ValueType * l_in_first = in_first.local();
      ValueType * l_in_last  = l_in_first + num_local_elem;
      out_last = std::copy(l_in_first,
                           l_in_last,
                           out_first);
    }
    DASH_LOG_TRACE("dash::copy", "finished local copy of",
                   (out_last - out_first), "elements");
    return out_last;
  }

  DASH_LOG_TRACE("dash::copy", "local range:",
                 li_range_in.begin,
                 li_range_in.end,
                 "in_first.is_local:", in_first.is_local());
  // Check if global input range is partially local:
  if (num_local_elem > 0) {
    // Part of the input range is local, copy local input subrange to
    // local output range directly.
    auto pattern          = in_first.pattern();
    // Map input iterators to global index domain:
    auto g_in_first       = in_first.global();
    auto g_in_last        = g_in_first + total_copy_elem;
    DASH_LOG_TRACE("dash::copy", "resolving local subrange");
    DASH_LOG_TRACE_VAR("dash::copy", num_local_elem);
    // Local index range to global input index range:
    // Global index of local range begin index:
    auto g_l_offset_begin = pattern.global(li_range_in.begin);
    // Global index of local range end index:
    //
    // NOTE: definition as pat.global(l_end-1)+1 as
    //       pat.global(l_end) would be out of range
    auto g_l_offset_end   = pattern.global(li_range_in.end-1) + 1;
    DASH_LOG_TRACE("dash::copy",
                   "global index range of local subrange:",
                   "begin:", g_l_offset_begin, "end:", g_l_offset_end);

    // Global position of input start iterator:
    auto g_offset_begin   = g_in_first.pos();
    // Convert local subrange to global iterators:
    auto g_l_in_first     = g_in_first +
                            (g_l_offset_begin - g_offset_begin);
    auto g_l_in_last      = g_in_first +
                            (g_l_offset_end   - g_offset_begin);
    DASH_LOG_TRACE("dash::copy", "global it. range of local subrange:",
                   "begin:", g_l_in_first.pos(),
                   "end:",   g_l_in_last.pos());
    DASH_LOG_TRACE_VAR("dash::copy", g_l_in_last.pos());

    auto num_prelocal_elem  = g_l_in_first.pos() - g_in_first.pos();
    auto num_postlocal_elem = in_last.pos() - g_l_offset_end;
    DASH_LOG_TRACE_VAR("dash::copy", num_prelocal_elem);
    DASH_LOG_TRACE_VAR("dash::copy", num_postlocal_elem);

    //
    // -------------------------------------------------------------------
    // Copy remote elements preceding the local subrange:
    //
    if (num_prelocal_elem > 0) {
      DASH_LOG_TRACE("dash::copy",
                     "copy global range preceding local subrange",
                     "g_in_first:", g_in_first.pos(),
                     "g_in_last:",  g_l_in_first.pos());
      // ... [ --- copy --- | ... l ... | ........ ]
      //     ^              ^           ^          ^
      //     in_first       l_in_first  l_in_last  in_last
      out_last = dash::internal::copy_block(g_in_first,
                                           g_l_in_first,
                                           dest_first);
      // Advance output pointers:
      dest_first = out_last;
    }
    //
    // -------------------------------------------------------------------
    // Copy local subrange:
    //
    // Convert local subrange of global input to native pointers:
    //
    // ... [ ........ | --- l --- | ........ ]
    //     ^          ^           ^          ^
    //     in_first   l_in_first  l_in_last  in_last
    //
    ValueType * l_in_first = g_l_in_first.local();
    ValueType * l_in_last  = l_in_first + num_local_elem;
    DASH_LOG_TRACE_VAR("dash::copy", l_in_first);
    DASH_LOG_TRACE_VAR("dash::copy", l_in_last);
    // Verify conversion of global input iterators to local pointers:
    DASH_ASSERT_MSG(l_in_first != nullptr,
                    "dash::copy: first index in global input (" <<
                    g_l_in_first.pos() << ") is not local");

    DASH_LOG_TRACE("dash::copy", "copy local subrange",
                   "num_copy_elem:", l_in_last - l_in_first);
    // Use memcpy for data ranges below 64 KB
    if (use_memcpy) {
      std::memcpy(dest_first, // destination
                  l_in_first, // source
                  num_local_elem * sizeof(ValueType));
      out_last = dest_first + num_local_elem;
    } else {
      out_last = std::copy(l_in_first,
                           l_in_last,
                           dest_first);
    }
    // Assert that all elements in local range have been copied:
    DASH_ASSERT_EQ(out_last, dest_first + num_local_elem,
                   "Expected to copy " << num_local_elem <<
                   " local elements "
                   "but copied " << (out_last - dest_first));
    DASH_LOG_TRACE("dash::copy", "finished local copy of",
                   (out_last - dest_first), "elements");
    // Advance output pointers:
    dest_first = out_last;
    //
    // -------------------------------------------------------------------
    // Copy remote elements succeeding the local subrange:
    //
    if (num_postlocal_elem > 0) {
      DASH_LOG_TRACE("dash::copy",
                     "copy global range succeeding local subrange",
                     "in_first:", g_l_in_last.pos(),
                     "in_last:",  g_in_last.pos());
      // ... [ ........ | ... l ... | --- copy --- ]
      //     ^          ^           ^              ^
      //     in_first   l_in_first  l_in_last      in_last
      out_last = dash::internal::copy_block(g_l_in_last,
                                           g_in_last,
                                           dest_first);
    }
  } else {
    DASH_LOG_TRACE("dash::copy", "no local subrange");
    // All elements in input range are remote
    out_last = dash::internal::copy_block(in_first,
                                         in_last,
                                         dest_first);
  }
  DASH_LOG_TRACE("dash::copy >", "finished,",
                 "out_last:", out_last);
  return out_last;
}


// ======================================================================
// Local to Global, Distributed Range
// ======================================================================

/**
 * Variant of \c dash::copy as asynchronous local-to-global copy
 * operation.
 * Output iterator type must specify dependent pattern type as
 * \c GlobOutputIt::pattern_type.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class ValueType,
  class GlobOutputIt >
auto copy_async(
  ValueType * in_first,
  ValueType * in_last,
  GlobOutputIt      out_first)
  -> typename std::enable_if<
       dash::has_type_pattern_type<GlobOutputIt>::value,
       dash::Future<GlobOutputIt>
     >::type
{
  DASH_LOG_TRACE("dash::copy_async()", "blocking, local to global");
  // Return value, initialize with begin of output range, indicating no
  // values have been copied:
  GlobOutputIt out_last   = out_first;
  // Number of elements to copy in total:
  auto num_elements       = std::distance(in_first, in_last);
  // Global iterator pointing at hypothetical end of output range:
  GlobOutputIt out_h_last = out_first + num_elements;

  DASH_LOG_TRACE_VAR("dash::copy_async", num_elements);
  // Futures of asynchronous get requests, one per block:
  auto futures    = std::vector< dash::Future<ValueType *> >();
  auto out_range  = dash::make_range(out_first, out_h_last);
  auto out_blocks = dash::blocks(out_range);
  auto in_copy_it = in_first;

  DASH_LOG_TRACE("dash::copy_async",
                 "number of blocks:", out_blocks.size());
  for (auto block : out_blocks) {
    DASH_LOG_TRACE("dash::copy_async", "block size:", block.size());
    futures.push_back(static_cast<dash::Future<GlobOutputIt>>(
                        dash::internal::copy_block_async(
                          in_copy_it,
                          in_copy_it + block.size(),
                          block.begin())) );
    in_copy_it += block.size();
  }
  out_last += dash::distance(in_first, in_copy_it);

  dash::Future<GlobOutputIt> fut_result([=]() mutable {
    auto _out = out_last;
    DASH_LOG_TRACE("dash::copy_async [Future]()",
                   "wait for", futures.size(), "async copy requests");
    DASH_LOG_TRACE("dash::copy_async [Future]", "  futures:", futures);
    DASH_LOG_TRACE("dash::copy_async [Future]", "  _out:", _out);
    for (auto f : futures) {
      f.wait();
    }
    DASH_LOG_TRACE("dash::copy_async [Future] >",
                   "async requests completed",
                   "futures:", futures, "_out:", _out);
    return _out;
  });
  DASH_LOG_TRACE("dash::copy_async >", "finished,",
                 "expected out_last:", out_last);
  return fut_result;
}

/**
 * Variant of \c dash::copy as asynchronous local-to-global copy
 * operation.
 * Output iterator type must specify dependent pattern type as
 * \c GlobOutputIt::pattern_type.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class ValueType,
  class GlobOutputIt >
auto copy_async(
  ValueType * in_first,
  ValueType * in_last,
  GlobOutputIt      out_first)
  -> typename std::enable_if<
       !dash::has_type_pattern_type<GlobOutputIt>::value,
       dash::Future<GlobOutputIt>
     >::type
{
  DASH_LOG_TRACE("dash::copy_async()",
                 "blocking, local to global, restricted to single block");
  return dash::internal::copy_block_async(
           in_first,
           in_last,
           out_first);
}

/**
 * Specialization of \c dash::copy as local-to-global blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class ValueType,
  class GlobOutputIt >
GlobOutputIt copy(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, local to global");
  // Return value, initialize with begin of output range, indicating no
  // values have been copied:
  GlobOutputIt out_last   = out_first;
  // Number of elements to copy in total:
  auto num_elements       = std::distance(in_first, in_last);
  // Global iterator pointing at hypothetical end of output range:
  GlobOutputIt out_h_last = out_first + num_elements;

  DASH_LOG_TRACE_VAR("dash::copy", num_elements);
  DASH_LOG_TRACE_VAR("dash::copy", out_first.pos());
  DASH_LOG_TRACE_VAR("dash::copy", out_h_last.pos());

  auto out_range  = dash::make_range(out_first, out_h_last);

  auto out_blocks = dash::blocks(out_range);
  auto in_copy_it = in_first;

  DASH_LOG_TRACE("dash::copy", "number of blocks:", out_blocks.size());
  for (auto block : out_blocks) {
    DASH_LOG_TRACE("dash::copy", "block size:", block.size());
    out_last   = static_cast<GlobOutputIt>(
                   dash::internal::copy_block(
                     in_copy_it,
                     in_copy_it + block.size(),
                     block.begin()));
    in_copy_it = in_first + dash::distance(out_first, out_last);
  }
  return out_last;
}

/**
 * Specialization of \c dash::copy as global-to-global blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  class GlobInputIt,
  class GlobOutputIt >
GlobOutputIt copy(
  GlobInputIt  in_first,
  GlobInputIt  in_last,
  GlobOutputIt out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to global");

  DASH_LOG_TRACE_VAR("dash::copy()", in_first);
  DASH_LOG_TRACE_VAR("dash::copy()", in_last);

  auto num_elements = dash::distance(in_first, in_last);
  DASH_LOG_TRACE_VAR("dash::copy()", num_elements);
  DASH_LOG_TRACE_VAR("dash::copy()", out_first);

  auto out_h_last   = out_first + num_elements;
  DASH_LOG_TRACE_VAR("dash::copy()", out_h_last);

  // in/out ranges in global domain:
  auto out_g_range  = dash::make_range(out_first, out_h_last);
  DASH_LOG_TRACE("dash::copy()", "range(out_gi, out_ge):",
                 dash::typestr(out_g_range));
  DASH_LOG_TRACE_VAR("dash::copy()", out_g_range);
  DASH_LOG_TRACE_VAR("dash::copy()", dash::index(out_g_range));

  auto in_g_range   = dash::make_range(in_first,  in_last);
  DASH_LOG_TRACE("dash::copy()", "range(in_gi, in_ge):",
                 dash::typestr(in_g_range));
  DASH_LOG_TRACE_VAR("dash::copy()", in_g_range);
  DASH_LOG_TRACE_VAR("dash::copy()", dash::index(in_g_range));

  auto in_blocks    = dash::blocks(in_g_range);
  DASH_LOG_TRACE_VAR("dash::copy()", in_blocks);

  auto l_in_blocks  = dash::local(in_blocks);
  DASH_LOG_TRACE_VAR("dash::copy()", in_blocks);

  auto out_blocks   = dash::blocks(out_g_range);
  DASH_LOG_TRACE_VAR("dash::copy()", out_blocks);

  // Iterator to active output block:
  auto out_block_it = out_blocks.begin();
  // Iterator local blocks in input range:
  for (auto l_in_block : l_in_blocks) {
    DASH_LOG_TRACE_VAR("dash::copy()", l_in_block);
  }

  // local view on in/out ranges:
  auto out_l_range  = dash::local(out_g_range);
  DASH_LOG_TRACE("dash::copy()", "local(range(out_gi, out_ge)):",
                 dash::typestr(out_l_range));
  DASH_LOG_TRACE("dash::copy()", "local(range(out_gi, out_ge)):",
                 out_l_range);
  DASH_LOG_TRACE("dash::copy()", "index(local(range(out_gi, out_ge))):",
                 dash::index(out_l_range));

  auto in_l_range   = dash::local(in_g_range);
  DASH_LOG_TRACE("dash::copy()", "local(range(in_gi, in_ge)):",
                 dash::typestr(in_l_range));
  DASH_LOG_TRACE("dash::copy()", "local(range(in_gi, in_ge)):",
                 in_l_range);
  DASH_LOG_TRACE("dash::copy()", "index(local(range(in_gi, in_ge))):",
                 dash::index(in_l_range));

  // Only sufficient if input- and output ranges have identical
  // decomposition:
  //
  //   auto out_l_end    = std::copy(dash::begin(in_l_range),
  //                                 dash::end(in_l_range),
  //                                 dash::begin(out_l_range));

  return out_first + num_elements;
}


// ======================================================================
// Other Specializations
// ======================================================================

#ifdef DASH_EXPERIMENTAL
/*
 * Specialization of \c dash::copy as global-to-local blocking copy
 * operation returning an allocated range.
 * Allows for zero-copy operations if the copied range is local.
 *
 * Returns a future of a local range { begin, end }.
 * If the requested data range is in shared memory, the range returned
 * references the native pointers of the target range.
 * If the requested data range needed to be copied from remote memory,
 * the range returned is the copied destination range such that
 * \c (begin = out_first).
 * and
 * \c (end = out_first + num_elem_copied.)
 */
template <
  typename ValueType,
  class GlobInputIt >
dash::Future< dash::LocalRange<ValueType> >
copy_async(
  GlobInputIt in_first,
  GlobInputIt in_last,
  ValueType * out_first)
{
  dash::LocalRange<ValueType> l_range;
  l_range.begin = nullptr;
  l_range.end   = nullptr;
  ValueType * l_in_first = in_first.local();
  ValueType * l_in_last  = (l_in_first == nullptr)
                           ? nullptr
                           : in_last.local();
  if (l_in_first != nullptr && l_in_last != nullptr) {
    l_range.begin = l_in_first;
    l_range.end   = l_in_last;
    return dash::Future< dash::LocalRange<ValueType> >(
             [=]() { return l_range; });
  }
  auto fut_copy_end = dash::copy_async(in_first, in_last, out_first);
  return dash::Future< dash::LocalRange<ValueType> >([=]() {
           l_range.begin = out_first;
           l_range.end   = fut_copy_end.get();
           return l_range;
         });
}
#endif

#endif // DOXYGEN

} // namespace dash

#endif // DASH__ALGORITHM__COPY_H__

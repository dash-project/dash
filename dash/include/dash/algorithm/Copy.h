#ifndef DASH__ALGORITHM__COPY_H__
#define DASH__ALGORITHM__COPY_H__

#include <dash/Future.h>
#include <dash/Iterator.h>
#include <dash/iterator/internal/ContiguousRange.h>

#include <dash/algorithm/LocalRange.h>

#include <dash/dart/if/dart_communication.h>

#include <algorithm>
#include <future>
#include <memory>
#include <vector>

namespace dash {

#ifdef DOXYGEN

/**
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::copy can be local (\c *ValueType) or global (\c GlobIter<ValueType>).
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
  InputIt  in_first,
  InputIt  in_last,
  OutputIt out_first);

/**
 * Asynchronous variant of \c dash::copy.
 * Copies the elements in the range, defined by \c [in_first, in_last), to
 * another range beginning at \c out_first.
 *
 * In terms of data distribution, source and destination ranges passed to
 * \c dash::copy can be local (\c *ValueType) or global (\c GlobIter<ValueType>).
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
 *           iterator that is created on completion of the asynchronous copy
 *           operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class GlobInputIt >
dash::Future<ValueType *> copy_async(
  InputIt  in_first,
  InputIt  in_last,
  OutputIt out_first);

#else // DOXYGEN

namespace internal {

// =========================================================================
// Global to Local
// =========================================================================

/**
 * Blocking implementation of \c dash::copy (global to local) without
 * optimization for local subrange.
 */
template <
  typename ValueType,
  class GlobInputIt >
ValueType * copy_impl(
  GlobInputIt                  begin,
  GlobInputIt                  end,
  ValueType                  * out_first,
  std::vector<dart_handle_t> & handles)
{
  DASH_LOG_TRACE("dash::copy_impl() global -> local",
                 "in_first:",  begin.pos(),
                 "in_last:",   end.pos(),
                 "out_first:", out_first);
  typedef typename GlobInputIt::pattern_type::size_type  size_type;
  size_type num_elem_total = dash::distance(begin, end);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy_impl", "input range empty");
    return out_first;
  }
  DASH_LOG_TRACE("dash::copy_impl",
                 "total elements:",    num_elem_total,
                 "expected out_last:", out_first + num_elem_total);

  size_type num_elem_copied = 0;

  ContiguousRangeSet<GlobInputIt> range_set{begin, end};

  //
  // Copy elements from every unit:
  //

  for (auto range : range_set) {

    auto cur_in_first  = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0,
                    "Number of element to copy is 0");
    auto dest_ptr = out_first + num_elem_copied;
    auto src_gptr = cur_in_first.dart_gptr();
    dart_handle_t handle;
    dash::internal::get_handle(src_gptr, dest_ptr, num_copy_elem, &handle);
    num_elem_copied += num_copy_elem;
    if (handle != DART_HANDLE_NULL) {
      handles.push_back(handle);
    }
  }

  DASH_ASSERT_EQ(num_elem_copied, num_elem_total,
                 "Failed to find all contiguous subranges in range");

  ValueType * out_last = out_first + num_elem_copied;
  DASH_LOG_TRACE_VAR("dash::copy_impl >", out_last);
  return out_last;
}

// =========================================================================
// Local to Global
// =========================================================================

/**
 * Blocking implementation of \c dash::copy (local to global) without
 * optimization for local subrange.
 */
template <
  typename ValueType,
  class GlobOutputIt >
GlobOutputIt copy_impl(
  ValueType                  * begin,
  ValueType                  * end,
  GlobOutputIt                 out_first,
  std::vector<dart_handle_t> & handles)
{

  DASH_LOG_TRACE("dash::copy_impl() local -> global",
                 "in_first:",  begin,
                 "in_last:",   end,
                 "out_first:", out_first);
  typedef typename GlobOutputIt::size_type  size_type;
  size_type num_elem_total = dash::distance(begin, end);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy_impl", "input range empty");
    return out_first;
  }

  auto out_last = out_first + num_elem_total;

  DASH_LOG_TRACE("dash::copy_impl",
                 "total elements:",    num_elem_total,
                 "expected out_last:", out_last);

  size_type num_elem_copied = 0;

  ContiguousRangeSet<GlobOutputIt> range_set{out_first, out_last};

  //
  // Copy elements from every unit:
  //

  for (auto range : range_set) {

    auto cur_in_first  = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0,
                    "Number of element to copy is 0");
    auto src_ptr  = begin + num_elem_copied;
    auto dst_gptr = out_first.dart_gptr();
    dart_handle_t handle;
    dash::internal::put_handle(dst_gptr, src_ptr, num_copy_elem, &handle);
    num_elem_copied += num_copy_elem;
    if (handle != DART_HANDLE_NULL) {
      handles.push_back(handle);
    }
  }

  DASH_ASSERT_EQ(num_elem_copied, num_elem_total,
                 "Failed to find all contiguous subranges in range");

  DASH_LOG_TRACE_VAR("dash::copy_impl >", out_last);
  return out_last;
}

} // namespace internal




// =========================================================================
// Global to Local, Distributed Range
// =========================================================================

/**
 * Variant of \c dash::copy as asynchronous global-to-local copy operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class    GlobInputIt >
dash::Future<ValueType *> copy_async(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType   * out_first)
{
  DASH_LOG_TRACE("dash::copy_async()", "async, global to local");
  if (in_first == in_last) {
    DASH_LOG_TRACE("dash::copy_async", "input range empty");
    return dash::Future<ValueType *>(out_first);
  }

  auto handles = std::make_shared<std::vector<dart_handle_t>>();

  auto out_last = internal::copy_impl(in_first, in_last, out_first, *handles);

  if (handles->empty()) {
    DASH_LOG_TRACE("dash::copy_async", "all transfers completed");
    return dash::Future<ValueType *>(out_first);
  }

  dash::Future<ValueType *> fut_result(
    // wait
    [=]() mutable {
      // Wait for all get requests to complete:
      ValueType * _out = out_last;
      DASH_LOG_TRACE("dash::copy_async_impl [Future]()",
                    "  wait for", handles->size(), "async get request");
      DASH_LOG_TRACE("dash::copy_async_impl [Future]", "  _out:", _out);
      if (!handles->empty()) {
        if (dart_waitall_local(handles->data(), handles->size())
            != DART_OK) {
          DASH_LOG_ERROR("dash::copy_async_impl [Future]",
                        "  dart_waitall_local failed");
          DASH_THROW(
            dash::exception::RuntimeError,
            "dash::copy_async_impl [Future]: dart_waitall_local failed");
        }
      } else {
        DASH_LOG_TRACE("dash::copy_async_impl [Future]", "  No pending handles");
      }
      DASH_LOG_TRACE("dash::copy_async_impl [Future] >",
                    "  async requests completed, _out:", _out);
      return _out;
    },
    // test
    [=](ValueType ** out) mutable {
      int32_t flag;
      DASH_ASSERT_RETURNS(
        DART_OK,
        dart_testall_local(handles->data(), handles->size(), &flag));
      if (flag) {
        handles->clear();
        *out = out_last;
      }
      return (flag != 0);
    },
    // destroy
    [=]() mutable {
      for (auto& handle : *handles) {
        DASH_ASSERT_RETURNS(
          DART_OK,
          dart_handle_free(&handle));
      }
    }
  );

  DASH_LOG_TRACE("dash::copy_async >", "finished,",
                 "expected out_last:", out_last);
  return fut_result;
}

/*
 * Specialization of \c dash::copy as global-to-local blocking copy operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class    GlobInputIt >
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

  // Check if part of the input range is local:
  DASH_LOG_TRACE_VAR("dash::copy", in_first.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", in_last.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", out_first);

  std::vector<dart_handle_t> handles;

  DASH_LOG_TRACE("dash::copy", "no local subrange");
  // All elements in input range are remote
  auto out_last = dash::internal::copy_impl(in_first,
                                            in_last,
                                            out_first,
                                            handles);

  if (!handles.empty()) {
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                  "num_handles: ", handles.size());
    dart_waitall_local(handles.data(), handles.size());
  }

  DASH_LOG_TRACE("dash::copy >", "finished,",
                 "out_last:", out_last);
  return out_last;
}


// =========================================================================
// Local to Global, Distributed Range
// =========================================================================

/**
 * Variant of \c dash::copy as asynchronous local-to-global copy operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
  typename ValueType,
  class GlobOutputIt >
dash::Future<GlobOutputIt> copy_async(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  auto handles  = std::make_shared<std::vector<dart_handle_t>>();
  auto out_last = dash::internal::copy_impl(in_first,
                                            in_last,
                                            out_first,
                                            *handles);

  if (handles->empty()) {
    return dash::Future<GlobOutputIt>(out_last);
  }
  dash::Future<GlobOutputIt> fut_result(
    // get
    [=]() mutable {
      // Wait for all get requests to complete:
      GlobOutputIt _out = out_last;
      DASH_LOG_TRACE("dash::copy_async [Future]()",
                    "  wait for", handles->size(), "async put request");
      DASH_LOG_TRACE("dash::copy_async [Future]", "  _out:", _out);
      if (!handles->empty()) {
        if (dart_waitall(handles->data(), handles->size())
            != DART_OK) {
          DASH_LOG_ERROR("dash::copy_async [Future]",
                        "  dart_waitall failed");
          DASH_THROW(
            dash::exception::RuntimeError,
            "dash::copy_async [Future]: dart_waitall failed");
        }
      } else {
        DASH_LOG_TRACE("dash::copy_async [Future]", "  No pending handles");
      }
      handles->clear();
      DASH_LOG_TRACE("dash::copy_async [Future] >",
                    "  async requests completed, _out:", _out);
      return _out;
    },
    // test
    [=](GlobOutputIt *out) mutable {
      int32_t flag;
      DASH_ASSERT_RETURNS(
        DART_OK,
        dart_testall(handles->data(), handles->size(), &flag));
      if (flag) {
        handles->clear();
        *out = out_last;
      }
      return (flag != 0);
    },
    // destroy
    [=]() mutable {
      for (auto& handle : *handles) {
        DASH_ASSERT_RETURNS(
          DART_OK,
          dart_handle_free(&handle));
      }
    }
  );
  return fut_result;
}

/**
 * Specialization of \c dash::copy as local-to-global blocking copy operation.
 *
 * \ingroup  DashAlgorithms
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
  // handles to wait on at the end
  std::vector<dart_handle_t> handles;
  // All elements in output range are remote
  DASH_LOG_TRACE("dash::copy", "no local subrange");
  auto out_last = dash::internal::copy_impl(in_first,
                                            in_last,
                                            out_first,
                                            handles);

  if (!handles.empty()) {
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                  "num_handles: ", handles.size());
    dart_waitall(handles.data(), handles.size());
  }

  return out_last;
}



// =========================================================================
// Other Specializations
// =========================================================================

#ifdef DASH_EXPERIMENTAL
/*
 * Specialization of \c dash::copy as global-to-local blocking copy operation
 * returning an allocated range.
 * Allows for zero-copy operations if the copied range is local.
 *
 * Returns a future of a local range { begin, end }.
 * If the requested data range is in shared memory, the range returned
 * references the native pointers of the target range.
 * If the requested data range needed to be copied from remote memory, the
 * range returned is the copied destination range such that begin = out_first
 * and end = out_first + num_elem_copied.
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

/**
 * Specialization of \c dash::copy as global-to-global blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <typename ValueType, class GlobInputIt, class GlobOutputIt>
GlobOutputIt copy(
    GlobInputIt /*in_first*/,
    GlobInputIt /*in_last*/,
    GlobOutputIt /*out_first*/)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to global");

  // TODO:
  // - Implement adapter for local-to-global dash::copy here
  // - Return if global input range has no local sub-range

  return GlobOutputIt();
}

#endif // DOXYGEN

} // namespace dash

#endif // DASH__ALGORITHM__COPY_H__

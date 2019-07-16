#ifndef DASH__ALGORITHM__COPY_H__
#define DASH__ALGORITHM__COPY_H__

#include <dash/Future.h>
#include <dash/Iterator.h>
#include <dash/internal/Config.h>
#include <dash/iterator/internal/ContiguousRange.h>

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

template<typename InputValueType, typename OutputValueType>
struct local_copy_chunk {
  using nonconst_input_value_type  = typename std::remove_const<InputValueType>::type;
  using nonconst_output_value_type = typename std::remove_const<OutputValueType>::type;

  const nonconst_input_value_type  *src;
        nonconst_output_value_type *dest;
  const size_t                      size;
};

template<typename InputValueType, typename OutputValueType>
using local_copy_chunks = std::vector<local_copy_chunk<InputValueType, OutputValueType>>;

template<typename InputValueType, typename OutputValueType>
void do_local_copies(local_copy_chunks<InputValueType, OutputValueType>& chunks)
{
  for (auto& chunk : chunks) {
    std::copy(chunk.src, chunk.src + chunk.size, chunk.dest);
  }
}

template<typename FromType, typename ToType>
struct is_dash_copyable
: std::integral_constant<bool,
      /* integral and floating point values are copyable if they have the same
       * size; but they are not convertible */
      (((std::is_integral<FromType>::value && std::is_integral<ToType>::value) ||
        (std::is_floating_point<FromType>::value &&
         std::is_floating_point<ToType>::value)) &&
        sizeof(FromType) == sizeof(ToType)) ||
      /* non-arithmetic types are not converted (except for cv-qualification) */
      std::is_same<typename std::remove_cv<FromType>::type,
                   typename std::remove_cv<ToType>::type>::value>
{ };


// =========================================================================
// Global to Local
// =========================================================================

/**
 * Blocking implementation of \c dash::copy (global to local) without
 * optimization for local subrange.
 */
template <
  typename ValueType,
  typename GlobInputIt >
ValueType * copy_impl(
  GlobInputIt                           begin,
  GlobInputIt                           end,
  ValueType                           * out_first,
  std::vector<dart_handle_t>          * handles,
  local_copy_chunks<
    typename GlobInputIt::value_type,
    ValueType>                        & local_chunks)
{
  DASH_LOG_TRACE("dash::internal::copy_impl() global -> local",
                 "in_first:",  begin.pos(),
                 "in_last:",   end.pos(),
                 "out_first:", out_first);
  typedef typename GlobInputIt::size_type   size_type;
  typedef typename GlobInputIt::value_type  input_value_type;
  typedef          ValueType                output_value_type;

  static_assert(is_dash_copyable<input_value_type, output_value_type>::value,
                "dash::copy can only be used on same-size arithmetic types or "
                "same non-arithmetic types");

  const size_type num_elem_total = dash::distance(begin, end);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::internal::copy_impl", "input range empty");
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

    auto cur_in        = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0, "Number of elements to copy is 0");
    auto dest_ptr = out_first + num_elem_copied;

    // handle local data locally
    if (cur_in.is_local()) {
      // if the chunk is less than a page or if it is the only transfer
      // don't bother post-poning it
      input_value_type* src_ptr = cur_in.local();
      if (num_elem_total == num_copy_elem ||
          DASH__ARCH__PAGE_SIZE > num_copy_elem*sizeof(input_value_type)) {
        std::copy(src_ptr, src_ptr + num_copy_elem, dest_ptr);
      } else {
        // larger chunks are handled later to allow overlap
        local_chunks.push_back({src_ptr, dest_ptr, num_copy_elem});
      }
    } else {
      auto src_gptr = cur_in.dart_gptr();

      DASH_LOG_TRACE("dash::copy_impl", "dest_ptr", dest_ptr,
                    "src_gptr", src_gptr, "num_copy_elem", num_copy_elem);

      if (handles != nullptr) {
        dart_handle_t handle;
        dash::internal::get_handle(src_gptr, dest_ptr, num_copy_elem, &handle);
        if (handle != DART_HANDLE_NULL) {
          handles->push_back(handle);
        }
      } else {
        dash::internal::get(src_gptr, dest_ptr, num_copy_elem);
      }
    }
    num_elem_copied += num_copy_elem;
  }

  DASH_ASSERT_EQ(num_elem_copied, num_elem_total,
                 "Failed to find all contiguous subranges in range");

  auto out_last = out_first + num_elem_copied;
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
  typename GlobOutputIt >
GlobOutputIt copy_impl(
  ValueType                           * begin,
  ValueType                           * end,
  GlobOutputIt                          out_first,
  std::vector<dart_handle_t>          * handles,
  local_copy_chunks<
    ValueType,
    typename GlobOutputIt::value_type> & local_chunks)
{
  DASH_LOG_TRACE("dash::copy_impl() local -> global",
                 "in_first:",  begin,
                 "in_last:",   end,
                 "out_first:", out_first);
  typedef typename GlobOutputIt::size_type  size_type;
  typedef typename GlobOutputIt::value_type output_value_type;
  typedef          ValueType                input_value_type;

  static_assert(is_dash_copyable<input_value_type, output_value_type>::value,
                "dash::copy can only be used on same-size arithmetic types or "
                "same non-arithmetic types");

  const size_type num_elem_total = dash::distance(begin, end);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::internal::copy_impl", "input range empty");
    return out_first;
  }

  auto out_last = out_first + num_elem_total;

  DASH_LOG_TRACE("dash::copy_impl",
                 "total elements:",    num_elem_total,
                 "expected out_last:", out_last);

  size_type num_elem_copied = 0;

  ContiguousRangeSet<GlobOutputIt> range_set{out_first, out_last};

  auto in_first = begin;

  //
  // Copy elements to every unit:
  //

  for (auto range : range_set) {

    auto cur_out_first  = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0,
                    "Number of elements to copy is 0");
    input_value_type* src_ptr  = in_first + num_elem_copied;

    // handle local data locally
    if (cur_out_first.is_local()) {
      output_value_type* dest_ptr = cur_out_first.local();
      // if the chunk is less than a page or if it is the only transfer
      // don't bother post-poning it
      if (num_elem_total == num_copy_elem ||
          DASH__ARCH__PAGE_SIZE > num_copy_elem*sizeof(input_value_type)) {
        std::copy(src_ptr, src_ptr + num_copy_elem, dest_ptr);
      } else {
        // larger chunks are handled later to allow overlap
        local_chunks.push_back({src_ptr, dest_ptr, num_copy_elem});
      }
    } else {
      auto dst_gptr = cur_out_first.dart_gptr();
      DASH_LOG_TRACE("dash::copy_impl", "src_ptr", src_ptr,
                    "dst_gptr", dst_gptr, "num_copy_elem", num_copy_elem);
      if (handles != nullptr) {
        dart_handle_t handle;
        dash::internal::put_handle(dst_gptr, src_ptr, num_copy_elem, &handle);
        if (handle != DART_HANDLE_NULL) {
          handles->push_back(handle);
        }
      } else {
        dash::internal::put(dst_gptr, src_ptr, num_copy_elem);
      }
    }
    num_elem_copied += num_copy_elem;
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
  dash::internal::local_copy_chunks<typename GlobInputIt::value_type, ValueType> local_chunks;
  auto out_last = dash::internal::copy_impl(in_first, in_last, out_first,
                                            handles.get(), local_chunks);
  dash::internal::do_local_copies(local_chunks);

  if (handles->empty()) {
    DASH_LOG_TRACE("dash::copy_async", "all transfers completed");
    return dash::Future<ValueType *>(out_last);
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
        dart_testall_local(handles->data(), handles->size(), &flag), DART_OK);
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
          dart_handle_free(&handle), DART_OK);
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
  class    GlobInputIt,
  bool     UseHandles = false >
ValueType * copy(
  GlobInputIt   in_first,
  GlobInputIt   in_last,
  ValueType   * out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to local");

  DASH_LOG_TRACE_VAR("dash::copy", in_first.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", in_last.dart_gptr());
  DASH_LOG_TRACE_VAR("dash::copy", out_first);

  if (in_first == in_last) {
    DASH_LOG_TRACE("dash::copy", "input range empty");
    return out_first;
  }

  ValueType *out_last;
  dash::internal::local_copy_chunks<typename GlobInputIt::value_type, ValueType> local_chunks;
  if (UseHandles) {
    std::vector<dart_handle_t> handles;
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first,
                                         &handles,
                                         local_chunks);
    if (!handles.empty()) {
      DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                    "num_handles: ", handles.size());
      dart_waitall_local(handles.data(), handles.size());
    }
    dash::internal::do_local_copies(local_chunks);

  } else {
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first,
                                         nullptr,
                                         local_chunks);
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete");
    dash::internal::do_local_copies(local_chunks);
    dart_flush_local_all(in_first.dart_gptr());
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
  typename GlobOutputIt >
dash::Future<GlobOutputIt> copy_async(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  if (in_first == in_last) {
    DASH_LOG_TRACE("dash::copy_async", "input range empty");
    return dash::Future<GlobOutputIt>(out_first);
  }

  auto handles  = std::make_shared<std::vector<dart_handle_t>>();
  dash::internal::local_copy_chunks<ValueType, typename GlobOutputIt::value_type> local_chunks;
  auto out_last = dash::internal::copy_impl(in_first,
                                            in_last,
                                            out_first,
                                            handles.get(),
                                            local_chunks);
  dash::internal::do_local_copies(local_chunks);

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
        dart_testall(handles->data(), handles->size(), &flag), DART_OK);
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
          dart_handle_free(&handle), DART_OK);
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
  typename GlobOutputIt,
  bool     UseHandles = false >
GlobOutputIt copy(
  ValueType    * in_first,
  ValueType    * in_last,
  GlobOutputIt   out_first)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, local to global");
  // handles to wait on at the end
  GlobOutputIt out_last;
  dash::internal::local_copy_chunks<ValueType, typename GlobOutputIt::value_type> local_chunks;
  if (UseHandles) {
    std::vector<dart_handle_t> handles;
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first,
                                         &handles,
                                         local_chunks);
    dash::internal::do_local_copies(local_chunks);

    if (!handles.empty()) {
      DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                    "num_handles: ", handles.size());
      dart_waitall(handles.data(), handles.size());
    }
  } else {
    out_last = dash::internal::copy_impl(in_first,
                                         in_last,
                                         out_first,
                                         nullptr,
                                         local_chunks);
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete");
    dash::internal::do_local_copies(local_chunks);
    dart_flush_all(out_first.dart_gptr());
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

struct ActiveDestination{};
struct ActiveSource{};

/**
 * Specialization of \c dash::copy as global-to-global blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
    class GlobInputIt,
    class GlobOutputIt,
    bool  UseHandles = false>
GlobOutputIt copy(
    GlobInputIt       in_first,
    GlobInputIt       in_last,
    GlobOutputIt      out_first,
    ActiveDestination /*unused*/)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to global, active destination");

  using size_type         = typename GlobInputIt::size_type;
  using input_value_type  = typename GlobInputIt::value_type;
  using output_value_type = typename GlobOutputIt::value_type;

  size_type num_elem_total = dash::distance(in_first, in_last);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy", "input range empty");
    return out_first;
  }

  auto g_out_first = out_first;
  auto g_out_last  = g_out_first + num_elem_total;

  internal::ContiguousRangeSet<GlobOutputIt> range_set{g_out_first, g_out_last};

  const auto & out_team = out_first.team();
  out_team.barrier();

  std::vector<dart_handle_t>  handles;
  std::vector<dart_handle_t>* handles_arg = UseHandles ? &handles : nullptr;

  dash::internal::local_copy_chunks<input_value_type, output_value_type> local_chunks;

  size_type num_elem_processed = 0;

  for (auto range : range_set) {

    auto cur_out_first  = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0,
                   "Number of elements to copy is 0");

    // handle local data only
    if (cur_out_first.is_local()) {
      auto dest_ptr = cur_out_first.local();
      auto src_ptr  = in_first + num_elem_processed;
      internal::copy_impl(src_ptr,
                          src_ptr + num_copy_elem,
                          dest_ptr,
                          handles_arg,
                          local_chunks);
    }
    num_elem_processed += num_copy_elem;
  }

  dash::internal::do_local_copies(local_chunks);

  if (!handles.empty()) {
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                  "num_handles: ", handles.size());
    dart_waitall_local(handles.data(), handles.size());
  } else if (!UseHandles) {
    dart_flush_local_all(in_first.dart_gptr());
  }
  out_team.barrier();

  DASH_ASSERT_EQ(num_elem_processed, num_elem_total,
                 "Failed to find all contiguous subranges in range");

  return g_out_last;
}

/**
 * Specialization of \c dash::copy as global-to-global blocking copy
 * operation.
 *
 * \ingroup  DashAlgorithms
 */
template <
    class GlobInputIt,
    class GlobOutputIt,
    bool  UseHandles = false>
GlobOutputIt copy(
    GlobInputIt  in_first,
    GlobInputIt  in_last,
    GlobOutputIt out_first,
    ActiveSource /*unused*/)
{
  DASH_LOG_TRACE("dash::copy()", "blocking, global to global");

  using size_type         = typename GlobInputIt::size_type;
  using input_value_type  = typename GlobInputIt::value_type;
  using output_value_type = typename GlobOutputIt::value_type;

  size_type num_elem_total = dash::distance(in_first, in_last);
  if (num_elem_total <= 0) {
    DASH_LOG_TRACE("dash::copy", "input range empty");
    return out_first;
  }

  internal::ContiguousRangeSet<GlobOutputIt> range_set{in_first, in_last};

  const auto & in_team = in_first.team();
  in_team.barrier();

  std::vector<dart_handle_t>             handles;
  std::vector<dart_handle_t>* handles_arg = UseHandles ? &handles : nullptr;

  dash::internal::local_copy_chunks<input_value_type, output_value_type> local_chunks;

  size_type num_elem_processed = 0;

  for (auto range : range_set) {

    auto cur_in_first  = range.first;
    auto num_copy_elem = range.second;

    DASH_ASSERT_GT(num_copy_elem, 0,
                   "Number of elements to copy is 0");

    // handle local data only
    if (cur_in_first.is_local()) {
      auto src_ptr = cur_in_first.local();
      auto dest_ptr  = out_first + num_elem_processed;
      internal::copy_impl(src_ptr,
                          src_ptr + num_copy_elem,
                          dest_ptr,
                          handles_arg,
                          local_chunks);
    }
    num_elem_processed += num_copy_elem;
  }

  internal::do_local_copies(local_chunks);

  if (!handles.empty()) {
    DASH_LOG_TRACE("dash::copy", "Waiting for remote transfers to complete,",
                  "num_handles: ", handles.size());
    dart_waitall(handles.data(), handles.size());
  } else if (!UseHandles) {
    dart_flush_all(out_first.dart_gptr());
  }
  in_team.barrier();

  DASH_ASSERT_EQ(num_elem_processed, num_elem_total,
                 "Failed to find all contiguous subranges in range");

  return out_first + num_elem_total;
}

#endif // DOXYGEN

} // namespace dash

#endif // DASH__ALGORITHM__COPY_H__

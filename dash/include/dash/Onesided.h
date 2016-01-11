#ifndef DASH__ONESIDED_H__
#define DASH__ONESIDED_H__

#include <dash/dart/if/dart.h>
#include <dash/GlobPtr.h>
#include <dash/Team.h>

namespace dash {

/**
 * Block until completion of local and global operations on a global
 * address.
 */
template<typename T>
void fence(
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_fence(gptr.dart_gptr()),
    DART_OK);
}

/**
 * Block until completion of local operations on a global address.
 */
template<typename T>
void fence_local(
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_fence_local(gptr.dart_gptr()),
    DART_OK);
}

/**
 * Write a value to a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T>
void put_value_nonblock(
  /// [IN]  Value to set
  const T & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_put(gptr.dart_gptr(),
             (void *)(&newval),
             sizeof(T)),
    DART_OK);
}

/**
 * Read a value fom a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T>
void get_value_nonblock(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T * ptr,
  /// [IN]  Global pointer to read
  const GlobPtr<T> & gptr
) {
  DASH_ASSERT_RETURNS(
    dart_get(ptr,
             gptr.dart_gptr(),
             sizeof(T)),
    DART_OK);
}

/**
 * Write a value to a global pointer
 *
 * \blocking
 */
template<typename T>
void put_value(
  /// [IN]  Value to set
  const T & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtr<T> & gptr)
{
  DASH_LOG_TRACE_VAR("dash::put_value()", gptr);
  DASH_ASSERT_RETURNS(
    dart_put_blocking(gptr.dart_gptr(),
                      (void *)(&newval),
                      sizeof(T)),
    DART_OK);
}

/**
 * Read a value fom a global pointer.
 *
 * \blocking
 */
template<typename T>
void get_value(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T * ptr,
  /// [IN]  Global pointer to read
  const GlobPtr<T> & gptr)
{
  DASH_LOG_TRACE_VAR("dash::get_value()", gptr);
  DASH_ASSERT_RETURNS(
    dart_get_blocking(ptr,
                      gptr.dart_gptr(),
                      sizeof(T)),
    DART_OK);
}

} // namespace dash

#endif // DASH__ONESIDED_H__

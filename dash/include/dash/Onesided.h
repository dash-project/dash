#ifndef DASH__ONESIDED_H__
#define DASH__ONESIDED_H__

#include <dash/Team.h>

#include <dash/dart/if/dart.h>


namespace dash {

/**
 * Block until completion of local and global operations on a global
 * address.
 */
template<typename T, typename GlobPtrType>
void fence(
  const GlobPtrType & gptr)
{
  DASH_ASSERT_RETURNS(
    dart_fence(gptr.dart_gptr()),
    DART_OK);
}

/**
 * Block until completion of local operations on a global address.
 */
template<typename T, typename GlobPtrType>
void fence_local(
  const GlobPtrType & gptr)
{
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
template<typename T, typename GlobPtrType>
void put_value_async(
  /// [IN]  Value to set
  const T           & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtrType & gptr)
{
  dash::dart_storage<T> ds(1);
  DASH_ASSERT_RETURNS(
    dart_put(gptr.dart_gptr(),
             (void *)(&newval),
             ds.nelem,
             ds.dtype),
    DART_OK);
}

/**
 * Read a value fom a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T, typename GlobPtrType>
void get_value_async(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T                 * ptr,
  /// [IN]  Global pointer to read
  const GlobPtrType & gptr)
{
  dash::dart_storage<T> ds(1);
  DASH_ASSERT_RETURNS(
    dart_get(ptr,
             gptr.dart_gptr(),
             ds.nelem,
             ds.dtype),
    DART_OK);
}

/**
 * Write a value to a global pointer
 *
 * \blocking
 */
template<typename T, typename GlobPtrType>
void put_value(
  /// [IN]  Value to set
  const T           & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtrType & gptr)
{
  dash::dart_storage<T> ds(1);
  DASH_ASSERT_RETURNS(
    dart_put_blocking(gptr.dart_gptr(),
                      (void *)(&newval),
                      ds.nelem,
                      ds.dtype),
    DART_OK);
}

/**
 * Read a value fom a global pointer.
 *
 * \blocking
 */
template<typename T, typename GlobPtrType>
void get_value(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T                 * ptr,
  /// [IN]  Global pointer to read
  const GlobPtrType & gptr)
{
  dash::dart_storage<T> ds(1);
  DASH_ASSERT_RETURNS(
    dart_get_blocking(ptr,
                      gptr.dart_gptr(),
                      ds.nelem,
                      ds.dtype),
    DART_OK);
}

} // namespace dash

#endif // DASH__ONESIDED_H__

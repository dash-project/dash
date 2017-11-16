#ifndef DASH__ONESIDED_H__
#define DASH__ONESIDED_H__

#include <dash/Team.h>

#include <dash/dart/if/dart.h>


namespace dash {

namespace internal {

  template<typename T>
  inline
  void
  put(const T *src, const dart_gptr_t& gptr, size_t nelem) {
    dash::dart_storage<T> ds(nelem);
    DASH_ASSERT_RETURNS(
      dart_put(gptr,
               src,
               ds.nelem,
               ds.dtype,
               ds.dtype),
      DART_OK);
  }

  template<typename T>
  inline
  void
  get(const dart_gptr_t& gptr, T *dst, size_t nelem) {
    dash::dart_storage<T> ds(nelem);
    DASH_ASSERT_RETURNS(
      dart_get(gptr,
               dst,
               ds.nelem,
               ds.dtype,
               ds.dtype),
      DART_OK);
  }

  template<typename T>
  inline
  void
  put_blocking(const T *src, const dart_gptr_t& gptr, size_t nelem) {
    dash::dart_storage<T> ds(nelem);
    DASH_ASSERT_RETURNS(
      dart_put_blocking(gptr,
                        src,
                        ds.nelem,
                        ds.dtype,
                        ds.dtype),
      DART_OK);
  }

  template<typename T>
  inline
  void
  get_blocking(const dart_gptr_t& gptr, T *dst, size_t nelem) {
    dash::dart_storage<T> ds(nelem);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(gptr,
                        dst,
                        ds.nelem,
                        ds.dtype,
                        ds.dtype),
      DART_OK);
  }

} // namespace internal

/**
 * Block until local and global completion of operations on a global
 * address.
 */
template<typename T, typename GlobPtrType>
void fence(
  const GlobPtrType & gptr)
{
  DASH_ASSERT_RETURNS(
    dart_flush(gptr.dart_gptr()),
    DART_OK);
}

/**
 * Block until local completion of operations on a global address.
 */
template<typename T, typename GlobPtrType>
void fence_local(
  const GlobPtrType & gptr)
{
  DASH_ASSERT_RETURNS(
    dart_flush_local(gptr.dart_gptr()),
    DART_OK);
}

/**
 * Write a value to a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T, typename GlobPtrType>
constexpr
void put_value_async(
  /// [IN]  Value to set
  const T           & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtrType & gptr)
{
  dash::internal::put(&newval, gptr.dart_gptr(), 1);
}

/**
 * Read a value fom a global pointer, non-blocking. Requires a later
 * fence operation to guarantee local and/or remote completion.
 *
 * \nonblocking
 */
template<typename T, typename GlobPtrType>
constexpr
void get_value_async(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T                 * ptr,
  /// [IN]  Global pointer to read
  const GlobPtrType & gptr)
{
  dash::internal::get(gptr.dart_gptr(), ptr, 1);
}

/**
 * Write a value to a global pointer
 *
 * \blocking
 */
template<typename T, typename GlobPtrType>
constexpr
void put_value(
  /// [IN]  Value to set
  const T           & newval,
  /// [IN]  Global pointer referencing target address of value
  const GlobPtrType & gptr)
{
  dash::internal::put_blocking(&newval, gptr.dart_gptr(), 1);
}

/**
 * Read a value fom a global pointer.
 *
 * \blocking
 */
template<typename T, typename GlobPtrType>
constexpr
void get_value(
  /// [OUT] Local pointer that will contain the value of the
  ///       global address
  T                 * ptr,
  /// [IN]  Global pointer to read
  const GlobPtrType & gptr)
{
  dash::internal::get_blocking(gptr.dart_gptr(), ptr, 1);
}

} // namespace dash

#endif // DASH__ONESIDED_H__

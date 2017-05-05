#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>
#include <dash/Allocator.h>
#include <dash/Future.h>
#include <dash/memory/GlobStaticMem.h>

#include <iostream>
#include <memory>

namespace dash {

/**
 * Global value reference for asynchronous / non-blocking write operations.
 * This is a write-only reference, asynchronous reads can be performed through
 * \c dash::Future<dash::GlobRef<T>>.
 *
 * Example:
 * \code
 *   array[0]       = 123;
 *   array.async[0] = 456;
 *   // Changes are not published immediately but the state is undefined
 *   assert(array[0] == 456); // not guaranteed to succeed
 *   // Changes on a container can be published in bulk:
 *   array.flush();
 *   assert(array[0] == 456); // guaranteed to succeed
 *   // From here, all changes are published
 *
 *   // Operations can be performed on GlobAsyncRef as well:
 *   auto garef = array.async[0];
 *        garef = 789;
 *   // Changes are not published immediately but the state is undefined
 *   assert(array[0] == 789); // not guaranteed to succeed
 *   // Changes on a GlobAsyncRef can be flushed using the reference:
 *   garef.flush();
 *   assert(array[0] == 789); // guaranteed to succeed
 *   // From here, all changes are published
 *
 *   // Asynchronous access is performed through dash::Future:
 *   dash::Future<dash::GlobRef<int>> fut = garef;
 *   // or simply:
 *   // auto fut = dash::Future(garef);
 *   // test for transfer to be complete
 *   if (!fut.test()) {
 *     // wait for the tranfer to complete
 *     fut.wait();
 *   }
 *   // access the result, previous wait not necessary
 *   assert(fut.get() == 789);
 * \endcode
 */
template<typename T>
class GlobAsyncRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobAsyncRef<U> & gar);

  template <
    typename ElementT >
  friend class GlobAsyncRef;

private:
  typedef GlobAsyncRef<T>
    self_t;

  typedef typename std::remove_const<T>::type
    nonconst_value_type;

  typedef typename std::add_const<T>::type
    const_value_type;

private:
  /// Pointer to referenced element in global memory
  dart_gptr_t  _gptr        = DART_GPTR_NULL;
  /// Pointer to referenced element in local memory
  T *          _lptr        = nullptr;
  /// Whether the referenced element is located local memory
  bool         _is_local    = false;

private:

  /**
   * Constructor used to reference members in GlobAsyncRefs referencing
   * structs
   */
  template<typename ParentT>
  GlobAsyncRef(
    /// parent GlobAsyncRef referencing whole struct
    const GlobAsyncRef<ParentT> & parent,
    /// offset of member in struct
    size_t                        offset)
  : _gptr(parent._gptr),
    _lptr(parent._is_local ? reinterpret_cast<T*>(
                              reinterpret_cast<char*>(parent._lptr)+offset)
                           : nullptr),
    _is_local(parent._is_local)
  {
     DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_gptr, offset),
      DART_OK);
  }

public:

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  template<class ElementT, class MemSpaceT>
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobPtr<ElementT, MemSpaceT> & gptr)
  : _gptr(gptr.dart_gptr())
  {
    _is_local = gptr.is_local();
    if (_is_local) {
      _lptr      = (T*)(gptr);
    }
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    dart_gptr_t   dart_gptr)
  : _gptr(dart_gptr)
  {
    GlobConstPtr<T> gptr(dart_gptr);
    _is_local = gptr.is_local();
    if (_is_local) {
      _lptr      = (T*)(gptr);
    }
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT>
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    const GlobConstPtr<ElementT> & gptr)
  : GlobAsyncRef(gptr.dart_gptr())
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    const GlobRef<T> & gref)
  : GlobAsyncRef(gref.dart_gptr())
  { }

  /**
   * Like native references, global reference types cannot be copied.
   *
   * Default definition of copy constructor would conflict with semantics
   * of \c operator=(const self_t &).
   */
  GlobAsyncRef(const self_t & other) = delete;

  /**
   * Unlike native reference types, global reference types are moveable.
   */
  GlobAsyncRef(self_t && other)      = default;

  /**
   * Whether the referenced element is located in local memory.
   */
  inline bool is_local() const noexcept
  {
    return _is_local;
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template<typename MEMTYPE>
  GlobAsyncRef<MEMTYPE> member(size_t offs) const {
    return GlobAsyncRef<MEMTYPE>(*this, offs);
  }

  /**
   * Get the member via pointer to member
   */
  template<class MEMTYPE, class P=T>
  GlobAsyncRef<MEMTYPE> member(
    const MEMTYPE P::*mem) const {
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<MEMTYPE>(offs);
  }

  /**
   * Comparison operator, true if both GlobAsyncRef objects point to same
   * element in local / global memory.
   */
  bool operator==(const self_t & other) const noexcept
  {
    return (_lptr == other._lptr &&
            _gptr == other._gptr);
  }

  /**
   * Inequality comparison operator, true if both GlobAsyncRef objects point
   * to different elements in local / global memory.
   */
  template <class GlobRefT>
  constexpr bool operator!=(const GlobRefT & other) const noexcept
  {
    return !(*this == other);
  }

  friend void swap(GlobAsyncRef<T> a, GlobAsyncRef<T> b) {
    nonconst_value_type temp = static_cast<nonconst_value_type>(a);
    a = b;
    b = temp;
  }


  /**
   * Set the value referenced by this \c GlobAsyncRef to \c val.
   *
   * \see operator=
   */
  void set(const_value_type & val) {
    operator=(val);
  }

  /**
   * Value assignment operator, sets new value in local memory or calls
   * non-blocking put on remote memory. This operator is only used for
   * types which are comparable
   */
  self_t &
  operator=(const_value_type & new_value)
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=", _gptr);
    if (_is_local) {
      *_lptr = new_value;
    } else {
      dart_storage_t ds = dash::dart_storage<T>(1);
      DASH_ASSERT_RETURNS(
        dart_put_blocking_local(
          _gptr, static_cast<const void *>(&new_value), ds.nelem, ds.dtype),
        DART_OK
      );
    }
    return *this;
  }

  /**
   * Return the underlying DART pointer.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept {
    return _gptr;
  }


  /**
   * Start the transfer of a single element and return a \c dash::Future<T>
   * that can be used to wait for the transfer to finish and to retrieve
   * the result.
   *
   * \see dash::async_copy
   */
  dash::Future<nonconst_value_type> const
  get() {
    if (_is_local) {
      return dash::Future<nonconst_value_type>(*_lptr);
    } else {
      dart_storage_t ds = dash::dart_storage<T>(1);
      dart_handle_t handle;
      // this could have been a unique_ptr if it were C++14 :(
      std::shared_ptr<nonconst_value_type> valptr(new nonconst_value_type);
      DASH_ASSERT_RETURNS(
        dart_get_handle(valptr.get(), _gptr, ds.nelem, ds.dtype, &handle),
        DART_OK);
      dash::Future<nonconst_value_type> fut( [=](){
        DASH_ASSERT_RETURNS(
          dart_wait(handle),
          DART_OK);
        return *valptr;
      });
      return fut;
    }
  }

  /**
   * Start the transfer of a single element and return a \c dash::Future<T>
   * that can be used to wait for the transfer to finish and to retrieve
   * the result.
   *
   * \see dash::async_copy
   */
  operator dash::Future<nonconst_value_type>() {
    return get();
  }

  /**
   * Flush all pending asynchronous operations on this asynchronous reference
   * and invalidate cached copies.
   */
  void flush()
  {
    // perform a flush irregardless of whether the reference is local or not
    if (!DART_GPTR_ISNULL(_gptr)) {
      DASH_ASSERT_RETURNS(
        dart_flush(_gptr),
        DART_OK
      );
    }
  }

}; // class GlobAsyncRef

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobAsyncRef<T> & gar) {
  if (gar._is_local) {
    os << "dash::GlobAsyncRef(" << gar._lptr << ")";
  } else {
    os << "dash::GlobAsyncRef(" << gar._gptr << ")";
  }
  return os;
}

}  // namespace dash

#endif // DASH__GLOB_ASYNC_REF_H__

#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/GlobMem.h>

#include <iostream>

namespace dash {

/**
 * Global value reference for asynchronous / non-blocking operations.
 *
 * Example:
 * \code
 *   GlobAsyncRef<int> gar0 = array.async[0];
 *   GlobAsyncRef<int> gar1 = array.async[1];
 *   gar0 = 123;
 *   gar1 = 456;
 *   // Changes are is visible locally but not published to other
 *   // units, yet:
 *   assert(gar0 == 123);
 *   assert(gar1 == 456);
 *   assert(array[0] == 123);
 *   assert(array[1] == 456);
 *   assert(array.local[0] == 123);
 *   assert(array.local[1] == 456);
 *   // Changes can be published (committed) directly using a GlobAsyncRef
 *   // object:
 *   gar0.flush();
 *   // New value of array[0] is published to all units, array[1] is not
 *   // committed yet
 *   // Changes on a container can be publiched in bulk:
 *   array.flush();
 *   // From here, all changes are published
 * \endcode
 */
template<typename T>
class GlobAsyncRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobAsyncRef<U> & gar);

private:
  typedef GlobAsyncRef<T>
    self_t;
  typedef GlobMem<T, dash::allocator::CollectiveAllocator<T> >
    GlobMem_t;

private:
  /// Instance of GlobMem that issued this global reference
  GlobMem_t  * _globmem;
  /// Value of the referenced element, initially not loaded
  mutable T    _value;
  /// Pointer to referenced element in global memory
  dart_gptr_t  _gptr;
  /// Pointer to referenced element in local memory
  T *          _lptr        = nullptr;
  /// Whether the value of the reference has been changed
  bool         _has_changed = false;
  /// Whether the referenced element is located local memory
  bool         _is_local    = false;
  /// Whether the value of the referenced element is known
  mutable bool _has_value   = false;

public:
  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Instance of GlobMem that issued this global reference
    GlobMem_t * globmem,
    /// Pointer to referenced object in global memory
    T         * lptr)
  : _value(*lptr),
    _lptr(lptr),
    _is_local(true),
    _has_value(true)
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * local memory.
   */
  GlobAsyncRef(
    /// Pointer to referenced object in local memory
    T * lptr)
  : _value(*lptr),
    _lptr(lptr),
    _is_local(true),
    _has_value(true)
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  template<class PatternT>
  GlobAsyncRef(
    /// Instance of GlobMem that issued this global reference
    GlobMem_t            * globmem,
    /// Pointer to referenced object in global memory
    GlobPtr<T, PatternT> & gptr)
  : _gptr(gptr.dart_gptr()),
    _is_local(gptr.is_local())
  {
    if (_is_local) {
      _value     = *gptr;
      _lptr      = (T*)(gptr);
      _has_value = true;
    }
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  template<class PatternT>
  GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobPtr<T, PatternT> & gptr)
  : _gptr(gptr.dart_gptr()),
    _is_local(gptr.is_local())
  {
    if (_is_local) {
      _value     = *gptr;
      _lptr      = (T*)(gptr);
      _has_value = true;
    }
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Instance of GlobMem that issued this global reference
    GlobMem_t   * globmem,
    /// Pointer to referenced object in global memory
    dart_gptr_t   dart_gptr)
  : _gptr(dart_gptr)
  {
    GlobPtr<T> gptr(dart_gptr);
    _is_local = gptr.is_local();
    if (_is_local) {
      _value     = *gptr;
      _lptr      = (T*)(gptr);
      _has_value = true;
    }
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Pointer to referenced object in global memory
    dart_gptr_t dart_gptr)
  : _gptr(dart_gptr)
  {
    GlobPtr<T> gptr(dart_gptr);
    _is_local = gptr.is_local();
    if (_is_local) {
      _value     = *gptr;
      _lptr      = (T*)(gptr);
      _has_value = true;
    }
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Instance of GlobMem that issued this global reference
    GlobMem_t  * globmem,
    /// Pointer to referenced object in global memory
    GlobRef<T> & gref)
  : GlobAsyncRef(globmem, gref.gptr())
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobRef<T> & gref)
  : GlobAsyncRef(gref.dart_gptr())
  { }

  /**
   * Whether the referenced element is located in local memory.
   */
  inline bool is_local() const
  {
    return _is_local;
  }

  /**
   * Conversion operator to referenced element value.
   */
  operator T() const
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.T()", _gptr);
    if (!_is_local) {
      dart_storage_t ds = dash::dart_storage<T>(1);
      dart_get(static_cast<void *>(&_value), _gptr, ds.nelem, ds.dtype);
    }
    return _value;
  }

  /**
   * Comparison operator, true if both GlobAsyncRef objects points to same
   * element in local / global memory.
   */
  bool operator==(const self_t & other) const
  {
    return (_lptr == other._lptr &&
            _gptr == other._gptr);
  }

  /**
   * Value assignment operator, sets new value in local memory or calls
   * non-blocking put on remote memory.
   */
  self_t & operator=(const T & new_value)
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=", _gptr);
    // TODO: Comparison with current value could be inconsistent
    if (!_has_value || _value != new_value) {
      _value       = new_value;
      _has_changed = true;
      _has_value   = true;
      if (_is_local) {
        *_lptr = _value;
      } else {
        dart_storage_t ds = dash::dart_storage<T>(1);
        dart_put_blocking(
          _gptr, static_cast<const void *>(&_value), ds.nelem, ds.dtype);
      }
    }
    return *this;
  }

  /**
   * Value increment operator.
   */
  self_t & operator+=(const T & ref)
  {
    T val = operator T();
    val += ref;
    operator=(val);
    return *this;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    T val = operator T();
    ++val;
    operator=(val);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t result = *this;
    T val = operator T();
    ++val;
    operator=(val);
    return result;
  }

  /**
   * Value decrement operator.
   */
  self_t & operator-=(const T & ref)
  {
    T val = operator T();
    val  -= ref;
    operator=(val);
    return *this;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    T val = operator T();
    --val;
    operator=(val);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t result = *this;
    T val = operator T();
    --val;
    operator=(val);
    return result;
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

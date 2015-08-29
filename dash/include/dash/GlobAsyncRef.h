#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <dash/GlobPtr.h>

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
class GlobAsyncRef {
private:
  typedef GlobAsyncRef<T> self_t;

  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobAsyncRef<U> & gar);

public:
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
    _has_value(true) {
  }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobPtr<T> & gptr)
  : _gptr(gptr),
    _is_local(gptr.is_local()) {
    if (_is_local) {
      _value     = *gptr;
      _lptr      = (T*)(gptr);
      _has_value = true;
    }
  }

  /**
   * Publish change on referenced object to all units if its value has been
   * changed.
   */
  void flush() {
    if (_has_changed) {
      DASH_LOG_TRACE_VAR("GlobAsyncRef.flush()", _gptr);
      DASH_LOG_TRACE_VAR("GlobAsyncRef.flush", _value);
      dash::put_value(_value, _gptr);
    }
  }

  /**
   * Whether the referenced element is located in local memory.
   */
  bool is_local() const {
    return _is_local;
  }
  
  /**
   * Conversion operator to referenced element value.
   */
  operator T() const {
    if (!_has_value) {
      // Get value from global memory:
      dash::get_value(&_value, _gptr);
    }
    return _value;
  }

  /**
   * Comparison operator, true if both GlobAsyncRef objects points to same
   * element in local / global memory.
   */
  bool operator==(const self_t & other) const {
    return (_lptr == other._lptr && 
            _gptr == other._gptr);
  }

  /**
   * Value assignment operator, sets new value in local memory.
   */
  self_t & operator=(const T & new_value) {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=", _gptr);
    _value       = new_value;
    _has_changed = true;
    _has_value   = true;
    return *this;
  }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) {
    return *this = T(other);
  }

private:
  /// Value of the referenced element, initially not loaded
  T          _value;
  /// Pointer to referenced element in global memory
  GlobPtr<T> _gptr;
  /// Pointer to referenced element in local memory
  T *        _lptr        = nullptr;
  /// Whether the value of the reference has been changed
  bool       _has_changed = false;
  /// Whether the referenced element is located local memory
  bool       _is_local    = false;
  /// Whether the value of the referenced element is known
  bool       _has_value   = false;

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

#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/memory/GlobStaticMem.h>

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
  /// Value of the referenced element, initially not loaded
  mutable nonconst_value_type _value;
  /// Whether the referenced element is located local memory
  bool         _is_local    = false;
  /// Whether the value of the referenced element is known
  mutable bool _has_value   = false;

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
    _value(parent._has_value ? *(reinterpret_cast<T*>(
                                reinterpret_cast<char*>(&(parent._value))+offset))
                             : T()),
    _is_local(parent._is_local),
    _has_value(parent._has_value)
  {
     DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_gptr, offset),
      DART_OK);
  }

public:
  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * local memory.
   */
  explicit constexpr GlobAsyncRef(
    /// Pointer to referenced object in local memory
    nonconst_value_type * lptr)
  : _lptr(lptr),
    _value(*lptr),
    _is_local(true),
    _has_value(true)
  { }

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
   * Conversion operator to referenced element value.
   */
  operator nonconst_value_type() const
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.T()", _gptr);
    if (!_has_value) {
      if (_is_local) {
        _value = *_lptr;
      } else {
        dash::dart_storage<T> ds(1);
        DASH_ASSERT_RETURNS(
          dart_get_blocking(
            static_cast<void *>(&_value), _gptr, ds.nelem, ds.dtype),
          DART_OK
        );
      }
      _has_value = true;
    }
    return _value;
  }

  /**
   * Comparison operator, true if both GlobAsyncRef objects points to same
   * element in local / global memory.
   */
  bool operator==(const self_t & other) const noexcept
  {
    return (_lptr == other._lptr &&
            _gptr == other._gptr);
  }

  /**
   * Inequality comparison operator, true if both GlobAsyncRef objects points
   * to different elements in local / global memory.
   */
  template <class GlobRefT>
  constexpr bool operator!=(const GlobRefT & other) const noexcept
  {
    return !(*this == other);
  }

  /**
   * Value-based comparison operator, true if the value refernced by the
   * GlobAsyncRef object is equal to \c value.
   */
  constexpr bool operator==(const_value_type & value) const
  {
    return static_cast<T>(*this) == value;
  }

  /**
   * Value-based inequality comparison operator, true if the value refernced
   * by the GlobAsyncRef object is not equal to \c value.
   */
  constexpr bool operator!=(const nonconst_value_type & value) const
  {
    return !(*this == value);
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
   * Return the value referenced by this \c GlobAsyncRef.
   */
  nonconst_value_type get() const {
    return operator nonconst_value_type();
  }

  /**
   * Asynchronously write the value referenced by this \c GlobAsyncRef
   * into \c tptr.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * at which point the referenced value can be used.
   */
  void get(nonconst_value_type *tptr) const {
    if (_is_local) {
      *tptr = *_lptr;
    } else {
      dash::dart_storage<T> ds(1);
      dart_get(static_cast<void *>(tptr), _gptr, ds.nelem, ds.dtype);
    }
  }

  /**
   * Asynchronously write  the value referenced by this \c GlobAsyncRef
   * into \c tref.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * at which point the referenced value can be used.
   */
  void get(nonconst_value_type& tref) const {
    get(&tref);
  }

  /**
   * Asynchronously set the value referenced by this \c GlobAsyncRef
   * to the value pointed to by \c tptr.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * but the pointer \c tptr can be re-used immediately.
   */
  void put(const_value_type* tptr) {
    operator=(*tptr);
  }

  /**
   * Asynchronously set the value referenced by this \c GlobAsyncRef
   * to the value pointed to by \c tref.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * but the value referenced by \c tref can be re-used immediately.
   */
  void put(const_value_type& tref) {
    operator=(tref);
  }

  /**
   * Value assignment operator, sets new value in local memory or calls
   * non-blocking put on remote memory. This operator is only used for
   * types which are comparable
   */
  template<typename __T = T>
  typename std::enable_if<dash::has_operator_equal<__T>::value, self_t & >::type
  operator=(const_value_type & new_value)
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=", _gptr);
    // TODO: Comparison with current value could be inconsistent
    if (!_has_value || _value != new_value) {
      _value       = new_value;
      _has_value   = true;
      if (_is_local) {
        *_lptr = _value;
      } else {
        dash::dart_storage<T> ds(1);
        DASH_ASSERT_RETURNS(
          dart_put(
            _gptr, static_cast<const void *>(&_value), ds.nelem, ds.dtype),
          DART_OK
        );
      }
    }
    return *this;
  }

  /**
   * Value assignment operator, sets new value in local memory or calls
   * non-blocking put on remote memory. This operator is only used for
   * types which are not comparable
   */
  template<typename __T = T>
  typename std::enable_if<!dash::has_operator_equal<__T>::value, self_t & >::type
  operator=(const_value_type & new_value)
  {
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.=", _gptr);
    // TODO: Comparison with current value could be inconsistent
    if (!_has_value) {
      _value       = new_value;
      _has_value   = true;
      if (_is_local) {
        *_lptr = _value;
      } else {
        dash::dart_storage<T> ds(1);
        DASH_ASSERT_RETURNS(
          dart_put(
            _gptr, static_cast<const void *>(&_value), ds.nelem, ds.dtype),
          DART_OK
        );
      }
    }
    return *this;
  }

  /**
   * Value increment operator.
   */
  self_t & operator+=(const_value_type & ref)
  {
    T val = operator nonconst_value_type();
    val += ref;
    operator=(val);
    return *this;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    nonconst_value_type val = operator nonconst_value_type();
    ++val;
    operator=(val);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t              result = *this;
    nonconst_value_type val    = operator nonconst_value_type();
    ++val;
    operator=(val);
    return result;
  }

  /**
   * Value decrement operator.
   */
  self_t & operator-=(const_value_type & ref)
  {
    nonconst_value_type val = operator nonconst_value_type();
    val  -= ref;
    operator=(val);
    return *this;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    nonconst_value_type val = operator nonconst_value_type();
    --val;
    operator=(val);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t              result = *this;
    nonconst_value_type val    = operator nonconst_value_type();
    --val;
    operator=(val);
    return result;
  }


  /**
   * Multiplication operator.
   */
  self_t & operator*=(const_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   *= ref;
    operator=(val);
    return *this;
  }

  /**
   * Division operator.
   */
  self_t & operator/=(const_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   /= ref;
    operator=(val);
    return *this;
  }

  /**
   * Binary XOR operator.
   */
  self_t & operator^=(const_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   ^= ref;
    operator=(val);
    return *this;
  }

  /**
   * Return the underlying DART pointer.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept {
    return _gptr;
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
      // require a re-read upon next reference
      _has_value = false;
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

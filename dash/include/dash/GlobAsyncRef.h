#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/memory/GlobStaticMem.h>

#include <iostream>

namespace dash {


template<typename ReferenceT, typename TargetT>
struct add_const_from_type
{
  using type = TargetT;
};

template<typename ReferenceT, typename TargetT>
struct add_const_from_type<const ReferenceT, TargetT>
{
  using type = typename std::add_const<TargetT>::type;
};


/**
 * Global value reference for asynchronous / non-blocking operations.
 *
 * Example:
 * \code
 *   GlobAsyncRef<int> gar0 = array.async[0];
 *   GlobAsyncRef<int> gar1 = array.async[1];
 *   gar0 = 123;
 *   gar1 = 456;
 *   // Changes are not guaranteed to be visible locally
 *   int val = array.async[0].get(); // might yield the old value
 *   // Values can be read asynchronously, which will not block.
 *   // Instead, the value will be available after `flush()`.
 *   array.async[0].get(&val);
 *   // Changes can be published (committed) directly using a GlobAsyncRef
 *   // object:
 *   gar0.flush();
 *   // New value of array[0] is published to all units, array[1] is not
 *   // committed yet
 *   // Changes on a container can be published in bulk:
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

public:
  using value_type          = T;
  using const_value_type    = typename std::add_const<T>::type;
  using nonconst_value_type = typename std::remove_const<T>::type;
  using self_t              = GlobAsyncRef<T>;
  using const_type          = GlobAsyncRef<const_value_type>;
  using nonconst_type       = GlobAsyncRef<nonconst_value_type>;


private:
  /// Pointer to referenced element in global memory
  dart_gptr_t  _gptr;
  /// Temporary value required for non-blocking put
  mutable nonconst_value_type _value;
  /// DART handle for asynchronous transfers
  mutable dart_handle_t _handle = DART_HANDLE_NULL;

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
  : _gptr(parent._gptr)
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
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    dart_gptr_t   dart_gptr)
  : _gptr(dart_gptr)
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    const GlobRef<nonconst_value_type> & gref)
  : GlobAsyncRef(gref.dart_gptr())
  { }

  /**
   * Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    const GlobRef<const_value_type> & gref)
  : GlobAsyncRef(gref.dart_gptr())
  {
    static_assert(std::is_same<value_type, const_value_type>::value,
                  "Cannot create GlobAsyncRef<T> from GlobRef<const T>!");
  }

  /**
   * Implicit conversion to const.
   */
  operator GlobAsyncRef<const_value_type>() {
    return GlobAsyncRef<const_value_type>(_gptr);
  }

  /**
   * Excpliti conversion to non-const.
   */
  explicit
  operator GlobAsyncRef<nonconst_value_type>() {
    return GlobAsyncRef<nonconst_value_type>(_gptr);
  }

  /**
   * Like native references, global reference types cannot be copied.
   *
   * Default definition of copy constructor would conflict with semantics
   * of \c operator=(const self_t &).
   */
  GlobAsyncRef(const self_t & other) = delete;

  ~GlobAsyncRef() {
    if (_handle != DART_HANDLE_NULL) {
      dart_wait_local(&_handle);
    }
  }

  /**
   * Unlike native reference types, global reference types are moveable.
   */
  GlobAsyncRef(self_t && other)      = default;

  /**
   * Whether the referenced element is located in local memory.
   */
  inline bool is_local() const noexcept
  {
    return dash::internal::is_local(this->_gptr);
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template<typename MEMTYPE>
  GlobAsyncRef<typename dash::add_const_from_type<T, MEMTYPE>::type>
  member(size_t offs) const {
    return GlobAsyncRef<typename dash::add_const_from_type<T, MEMTYPE>::type>(*this, offs);
  }

  /**
   * Get the member via pointer to member
   */
  template<class MEMTYPE, class P=T>
  GlobAsyncRef<typename dash::add_const_from_type<T, MEMTYPE>::type>
  member(
    const MEMTYPE P::*mem) const {
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<typename dash::add_const_from_type<T, MEMTYPE>::type>(offs);
  }

  /**
   * Swap values with synchronous reads and asynchronous writes.
   */
  friend void swap(self_t & a, self_t & b) {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot swap GlobAsyncRef<const T>!");
    nonconst_value_type temp = a->get();
    a = b->get();
    b = temp;
  }

  /**
   * Return the value referenced by this \c GlobAsyncRef.
   * This operation will block until the value has been transfered.
   */
  nonconst_value_type get() const {
    nonconst_value_type value;
    DASH_LOG_TRACE_VAR("GlobAsyncRef.T()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(static_cast<void *>(&value), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
    return value;
  }

  /**
   * Asynchronously write the value referenced by this \c GlobAsyncRef
   * into \c tptr.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * at which point the referenced value can be used.
   */
  void get(nonconst_value_type *tptr) const {
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get(static_cast<void *>(tptr), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
  }

  /**
   * Asynchronously write the value referenced by this \c GlobAsyncRef
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
   * This operation is guaranteed to be complete after a call to \ref flush
   * and the pointer \c tptr should not be reused before completion.
   */
  void set(const_value_type* tptr) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value through GlobAsyncRef<const T>!");
    DASH_LOG_TRACE_VAR("GlobAsyncRef.set()", *tptr);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.set()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_put(_gptr, static_cast<const void *>(tptr), ds.nelem, ds.dtype),
      DART_OK
    );
  }

  /**
   * Asynchronously set the value referenced by this \c GlobAsyncRef
   * to the value pointed to by \c new_value.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * but the value referenced by \c new_value can be re-used immediately.
   */
  void set(const_value_type& new_value) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value through GlobAsyncRef<const T>!");
    DASH_LOG_TRACE_VAR("GlobAsyncRef.set()", new_value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef.set()", _gptr);
    dash::dart_storage<T> ds(1);
    _value = new_value;
    // check that we do not overwrite the handle if it has been used before
    if (this->_handle != DART_HANDLE_NULL) {
      DASH_ASSERT_RETURNS(
        dart_wait_local(&this->_handle),
        DART_OK
      );
    }
    DASH_ASSERT_RETURNS(
      dart_put_handle(_gptr, static_cast<const void *>(&_value),
                      ds.nelem, ds.dtype, &_handle),
      DART_OK
    );
  }

  /**
   * Value assignment operator, calls non-blocking put.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * but the value referenced by \c new_value can be re-used immediately.
   */
  const self_t &
  operator=(const_value_type & new_value) const
  {
    set(new_value);
    return *this;
  }

  /**
   * Returns the underlying DART global pointer.
   */
  dart_gptr_t dart_gptr() const {
    return this->_gptr;
  }

  /**
   * Flush all pending asynchronous operations on this asynchronous reference.
   */
  void flush() const
  {
    DASH_ASSERT_RETURNS(
      dart_flush(_gptr),
      DART_OK
    );
  }

}; // class GlobAsyncRef

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobAsyncRef<T> & gar) {
  os << "dash::GlobAsyncRef(" << gar._gptr << ")";
  return os;
}

}  // namespace dash

#endif // DASH__GLOB_ASYNC_REF_H__

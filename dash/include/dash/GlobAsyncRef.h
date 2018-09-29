#ifndef DASH__GLOB_ASYNC_REF_H__
#define DASH__GLOB_ASYNC_REF_H__

#include <iostream>

#include <dash/GlobPtr.h>
#include <dash/Onesided.h>

#include <dash/iterator/internal/GlobRefBase.h>


namespace dash {

// Forward declarations
template <class T>
class GlobRef;

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

  struct ReleaseHandle {
    void operator()(dart_handle_t handle)
    {
      DASH_ASSERT_RETURNS(
        dart_wait_local(&handle),
        DART_OK
      );
    }
  };

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
  mutable std::
      unique_ptr<std::remove_pointer<dart_handle_t>::type, ReleaseHandle>
          _handle{DART_HANDLE_NULL};

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

  /**
   * PRIVATE: Conctructor, creates an GlobRefAsync object referencing an element in
   * global memory.
   */
  template<class ElementT, class MemSpaceT>
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobPtr<ElementT, MemSpaceT> & gptr)
  : _gptr(gptr.dart_gptr())
  { }

public:

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
   * Copy constructor, implicit if at least one of the following conditions is
   * satisfied:
   *    1) value_type and _T are exactly the same types (including const and
   *    volatile qualifiers
   *    2) value_type and _T are the same types after removing const and volatile
   *    qualifiers and value_type itself is const.
   */
  template<typename _T,
           int = internal::enable_implicit_copy_ctor<value_type, _T>::value>
  GlobAsyncRef(const GlobAsyncRef<_T>& gref)
  : GlobAsyncRef(gref.dart_gptr())
  { }

  /**
   * Copy constructor, explicit if the following conditions are satisfied.
   *    1) value_type and _T are the same types after excluding const and
   *    volatile qualifiers
   *    2) value_type is const and _T is non-const
   */
  template <
      typename _T,
      long = internal::enable_explicit_copy_ctor<value_type, _T>::value>
  explicit GlobAsyncRef(const GlobAsyncRef<_T>& gref)
    : GlobAsyncRef(gref.dart_gptr())
  {
  }

  template <
      typename _T,
      int = internal::enable_implicit_copy_ctor<value_type, _T>::value>
  GlobAsyncRef(const GlobRef<_T>& gref)
    : GlobAsyncRef(gref.dart_gptr())
  {
  }

  template <
      typename _T,
      long = internal::enable_explicit_copy_ctor<value_type, _T>::value>
  explicit GlobAsyncRef(const GlobRef<_T>& gref)
    : GlobAsyncRef(gref.dart_gptr())
  {
  }

  ~GlobAsyncRef() = default;

  GlobAsyncRef(self_t&& other) = default;

  /**
   * MOVE Assignment
   */
  self_t& operator=(self_t&& other) = default;

  /**
   * Copy Assignment
   */
  self_t& operator=(const self_t& other) = delete;

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
  GlobAsyncRef<typename internal::add_const_from_type<T, MEMTYPE>::type>
  member(size_t offs) const {
    return GlobAsyncRef<typename internal::add_const_from_type<T, MEMTYPE>::type>(*this, offs);
  }

  /**
   * Get the member via pointer to member
   */
  template<class MEMTYPE, class P=T>
  GlobAsyncRef<typename internal::add_const_from_type<T, MEMTYPE>::type>
  member(
    const MEMTYPE P::*mem) const {
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<typename internal::add_const_from_type<T, MEMTYPE>::type>(offs);
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
    dash::internal::get_blocking(_gptr, &value, 1);
    return value;
  }

  /**
   * Asynchronously write the value referenced by this \c GlobAsyncRef
   * into \c tptr.
   * This operation is guaranteed to be complete after a call to \ref flush,
   * at which point the referenced value can be used.
   */
  void get(nonconst_value_type *tptr) const {
    dash::internal::get(_gptr, tptr, 1);
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
    dash::internal::put(_gptr, tptr, 1);
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

    _value = new_value;

    dart_handle_t new_handle;
    dash::internal::put_handle(_gptr, &_value, 1, &new_handle);

    _handle.reset(new_handle);
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
   * Disallow implicit comparison with other global references.
   */
  template <class ValueT>
  bool operator==(const GlobAsyncRef<ValueT> & other) = delete;

  template <class ValueT>
  bool operator!=(const GlobAsyncRef<ValueT> & other) = delete;

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

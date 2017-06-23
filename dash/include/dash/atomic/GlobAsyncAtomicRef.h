#ifndef DASH__ASYNC_ATOMIC_GLOBREF_H_
#define DASH__ASYNC_ATOMIC_GLOBREF_H_

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/GlobAsyncRef.h>
#include <dash/algorithm/Operation.h>


namespace dash {

// forward decls
template<typename T>
class Atomic;

template<typename T>
class Shared;

/**
 * Specialization for atomic values. All atomic operations are 
 * \c const as the \c GlobRef does not own the atomic values.
 */
template<typename T>
class GlobAsyncRef<dash::Atomic<T>>
{
  /* Notes on type compatibility:
   *
   * - The general support of atomic operations on values of type T is
   *   checked in `dash::Atomic` and is not verified here.
   * - Whether arithmetic operations (like `fetch_add`) are supported
   *   for values of type T is implicitly tested in the DASH operation
   *   types (like `dash::plus<T>`) and is not verified here.
   *
   */

  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobAsyncRef<U> & gref);

public:
  typedef T
    value_type;
  typedef GlobAsyncRef<const dash::Atomic<T>>
    const_type;

private:
  typedef dash::Atomic<T>         atomic_t;
  typedef GlobAsyncRef<atomic_t>  self_t;

private:
  dart_gptr_t _gptr;

public:
  /**
   * Default constructor, creates an GlobRef object referencing an element in
   * global memory.
   */
  GlobAsyncRef()
  : _gptr(DART_GPTR_NULL) {
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<typename PatternT>
  explicit GlobAsyncRef(
    /// Pointer to referenced object in global memory
    GlobPtr<atomic_t, PatternT> & gptr)
  : GlobAsyncRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<typename PatternT>
  GlobAsyncRef(
    /// Pointer to referenced object in global memory
    const GlobPtr<atomic_t, PatternT> & gptr)
  : GlobAsyncRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  explicit GlobAsyncRef(dart_gptr_t dart_gptr)
  : _gptr(dart_gptr)
  {
    DASH_LOG_TRACE_VAR("GlobRef(dart_gptr_t)", dart_gptr);
  }

  /**
   * Copy constructor.
   */
  GlobAsyncRef(
    /// GlobRef instance to copy.
    const GlobRef<atomic_t> & other)
  : _gptr(other._gptr)
  { }

  self_t & operator=(const self_t & other) = delete;

  inline bool operator==(const self_t & other) const noexcept
  {
    return _gptr == other._gptr;
  }

  inline bool operator!=(const self_t & other) const noexcept
  {
    return !(*this == other);
  }

  inline bool operator==(const T & value) const = delete;
  inline bool operator!=(const T & value) const = delete;

  operator T() const {
    return load();
  }

  operator GlobPtr<T>() const {
    DASH_LOG_TRACE("GlobRef.GlobPtr()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    return GlobPtr<atomic_t>(_gptr);
  }

  dart_gptr_t dart_gptr() const {
    return _gptr;
  }

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    return GlobPtr<T>(_gptr).is_local();
  }

  void flush() {
    dart_flush(_gptr);
  }

  void flush_local() {
    dart_flush_local(_gptr);
  }

  /**
   * Set the value of the shared atomic variable.
   */
  void set(const T * value) const
  {
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.store()", *value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.store",   _gptr);
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<const void * const>(value),
                       1,
                       dash::dart_punned_datatype<T>::value,
                       DART_OP_REPLACE);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG("GlobAsyncRef<Atomic>.store >");
  }

  /// atomically fetches value
  T get() const
  {
    DASH_LOG_DEBUG("GlobAsyncRef<Atomic>.load()");
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.load", _gptr);
    value_type nothing;
    value_type result;
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<void * const>(&nothing),
                       reinterpret_cast<void * const>(&result),
                       dash::dart_punned_datatype<T>::value,
                       DART_OP_NO_OP);
    flush_local();
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.get >", result);
    return result;
  }

  /// atomically fetches value
  void get(T * result) const
  {
    DASH_LOG_DEBUG("GlobAsyncRef<Atomic>.load()");
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.load", _gptr);
    value_type nothing;
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<void * const>(&nothing),
                       reinterpret_cast<void * const>(result),
                       dash::dart_punned_datatype<T>::value,
                       DART_OP_NO_OP);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
  }

  /**
   * Get the value of the shared atomic variable.
   */
  inline T load() const {
    return get();
  }

  /**
   * Atomically executes specified operation on the referenced shared value.
   */
  template<typename BinaryOp>
  void op(
    BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    const T * value) const
  {
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.op()", *value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.op",   _gptr);
    DASH_LOG_TRACE("GlobAsyncRef<Atomic>.op", "dart_accumulate");
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<const void *>(value),
                       1,
                       dash::dart_punned_datatype<T>::value,
                       binary_op.dart_operation());
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.op >", *value);
  }

  /**
   * Atomic fetch-and-op operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<typename BinaryOp>
  void fetch_op(
    BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    const T * value,
          T * result) const
  {
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.fetch_op()", *value);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.fetch_op",   _gptr);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.fetch_op",   typeid(*value).name());
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<const void * const>(value),
                       reinterpret_cast<void * const>(result),
                       dash::dart_punned_datatype<T>::value,
                       binary_op.dart_operation());
    DASH_ASSERT_EQ(DART_OK, ret, "dart_fetch_op failed");
  }

  /**
   * Atomically exchanges value
   */
  void exchange(const T * value, T * result) const {
    fetch_op(dash::second<T>(), value, result);
  }

  /**
   * Atomically compares the value with the value of expected and if thosei
   * are bitwise-equal, replaces the former with desired.
   * 
   * \return  True if value is exchanged
   * 
   * \see \c dash::atomic::compare_exchange
   */
  void compare_exchange(
    const T * expected,
    const T * desired,
          T * result) const {
    DASH_LOG_DEBUG_VAR("GlobAsyncRef<Atomic>.compare_exchange()", *desired);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.compare_exchange",   _gptr);
    DASH_LOG_TRACE_VAR("GlobAsyncRef<Atomic>.compare_exchange",   *expected);
    DASH_LOG_TRACE_VAR(
      "GlobAsyncRef<Atomic>.compare_exchange", typeid(*desired).name());
    dart_ret_t ret = dart_compare_and_swap(
                       _gptr,
                       reinterpret_cast<const void * const>(desired),
                       reinterpret_cast<const void * const>(expected),
                       reinterpret_cast<void * const>(result),
                       dash::dart_punned_datatype<T>::value);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_compare_and_swap failed");
  }

  /**
   * DASH specific variant which is faster than \c fetch_add
   * but does not return value
   */
  void add(const T * value) const
  {
    op(dash::plus<T>(), value);
  }

  /**
   * Atomic fetch-and-add operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  void fetch_add(
    /// Value to be added to global atomic variable.
    const T * value,
          T * result) const
  {
    fetch_op(dash::plus<T>(), value, result);
  }

};

} // namespace dash

#endif // DASH__ASYNC_ATOMIC_GLOBREF_H_


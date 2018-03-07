#ifndef DASH__ATOMIC_GLOBREF_H_
#define DASH__ATOMIC_GLOBREF_H_

#include <dash/GlobPtr.h>
#include <dash/Types.h>
#include <dash/algorithm/Operation.h>

namespace dash {

// forward decls
template <typename T>
class Atomic;

/**
 * Specialization for atomic values. All atomic operations are
 * \c const as the \c GlobRef does not own the atomic values.
 */
template <typename T>
class GlobRef<dash::Atomic<T>> {
  /* Notes on type compatibility:
   *
   * - The general support of atomic operations on values of type T is
   *   checked in `dash::Atomic` and is not verified here.
   * - Whether arithmetic operations (like `fetch_add`) are supported
   *   for values of type T is implicitly tested in the DASH operation
   *   types (like `dash::plus<T>`) and is not verified here.
   *
   */

  template <typename U>
  friend std::ostream &operator<<(std::ostream &os, const GlobRef<U> &gref);

public:
  using value_type          = T;
  using const_value_type    = typename std::add_const<T>::type;
  using nonconst_value_type = typename std::remove_const<T>::type;
  using atomic_t            = dash::Atomic<T>;
  using const_atomic_t      = typename dash::Atomic<const_value_type>;
  using nonconst_atomic_t   = typename dash::Atomic<nonconst_value_type>;
  using self_t              = GlobRef<atomic_t>;
  using const_type          = GlobRef<const_atomic_t>;
  using nonconst_type       = GlobRef<dash::Atomic<nonconst_value_type>>;

private:
  dart_gptr_t _gptr;

public:
  /**
   * Reference semantics forbid declaration without definition.
   */
  GlobRef() = delete;

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template <typename GlobMemT>
  explicit GlobRef(
      /// Pointer to referenced object in global memory
      GlobPtr<atomic_t, GlobMemT> &gptr)
    : GlobRef(gptr.dart_gptr())
  {
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template <typename GlobMemT>
  GlobRef(
      /// Pointer to referenced object in global memory
      const GlobPtr<const_atomic_t, GlobMemT> &gptr)
    : GlobRef(gptr.dart_gptr())
  {
    static_assert(
        std::is_same<value_type, const_value_type>::value,
        "Cannot create GlobRef<Atomic<T>> from GlobPtr<Atomic<const T>>!");
  }

  template <typename GlobMemT>
  GlobRef(
      /// Pointer to referenced object in global memory
      const GlobPtr<nonconst_atomic_t, GlobMemT> &gptr)
    : GlobRef(gptr.dart_gptr())
  {
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  explicit GlobRef(dart_gptr_t dart_gptr)
    : _gptr(dart_gptr)
  {
    DASH_LOG_TRACE_VAR("GlobRef(dart_gptr_t)", dart_gptr);
  }

  /**
   * Like native references, global reference types cannot be copied.
   *
   * Default definition of copy constructor would conflict with semantics
   * of \c operator=(const self_t &).
   */
  GlobRef(const self_t &other) = delete;

  /**
   * Unlike native reference types, global reference types are moveable.
   */
  GlobRef(self_t &&other) = default;

  self_t &operator=(const self_t &other) = delete;

  operator T() const
  {
    return load();
  }

  /**
   * Implicit conversion to const type.
   * memory.
   */
  operator const_type() const
  {
    return const_type(_gptr);
  }

  /**
   * Explicit conversion to non-const type.
   */
  explicit operator nonconst_type() const
  {
    return nonconst_type(_gptr);
  }

  dart_gptr_t dart_gptr() const
  {
    return _gptr;
  }

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const
  {
    return dash::internal::is_local(_gptr);
  }

  /**
   * atomically assigns value
   *
   * \return The assigned value.
   *
   * \note This operator does not return a reference but a copy of the value
   * in order to ensure atomicity. This is consistent with the C++ std::atomic
   * \c operator=, see
   * http://en.cppreference.com/w/cpp/atomic/atomic/operator%3D.
   */
  T operator=(const T &value) const
  {
    store(value);
    return value;
  }

  /**
   * Set the value of the shared atomic variable.
   */
  void set(const T &value) const
  {
    static_assert(
        std::is_same<value_type, nonconst_value_type>::value,
        "Cannot modify value referenced by GlobRef<Atomic<const T>>!");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.store()", value);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.store", _gptr);
    dart_ret_t ret = dart_accumulate(
        _gptr,
        reinterpret_cast<const void *const>(&value),
        1,
        dash::dart_punned_datatype<T>::value,
        DART_OP_REPLACE);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG("GlobRef<Atomic>.store >");
  }

  /**
   * Set the value of the shared atomic variable.
   */
  inline void store(const T &value) const
  {
    set(value);
  }

  /// atomically fetches value
  T get() const
  {
    DASH_LOG_DEBUG("GlobRef<Atomic>.load()");
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.load", _gptr);
    nonconst_value_type nothing;
    nonconst_value_type result;
    dart_ret_t          ret = dart_fetch_and_op(
        _gptr,
        reinterpret_cast<void *const>(&nothing),
        reinterpret_cast<void *const>(&result),
        dash::dart_punned_datatype<T>::value,
        DART_OP_NO_OP);
    dart_flush_local(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.get >", result);
    return result;
  }

  /**
   * Get the value of the shared atomic variable.
   */
  inline T load() const
  {
    return get();
  }

  /**
   * Atomically executes specified operation on the referenced shared value.
   */
  template <typename BinaryOp>
  void op(
      /// Binary operation to be performed on global atomic variable
      BinaryOp binary_op,
      /// Value to be used in binary op on global atomic variable.
      const T &value) const
  {
    static_assert(
        std::is_same<value_type, nonconst_value_type>::value,
        "Cannot modify value referenced by GlobRef<Atomic<const T>>!");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.op()", value);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.op", _gptr);
    nonconst_value_type acc = value;
    DASH_LOG_TRACE("GlobRef<Atomic>.op", "dart_accumulate");
    dart_ret_t ret = dart_accumulate(
        _gptr,
        reinterpret_cast<char *>(&acc),
        1,
        dash::dart_punned_datatype<T>::value,
        binary_op.dart_operation());
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.op >", acc);
  }

  /**
   * Atomic fetch-and-op operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template <typename BinaryOp>
  T fetch_op(
      BinaryOp binary_op,
      /// Value to be added to global atomic variable.
      const T &value) const
  {
    static_assert(
        std::is_same<value_type, nonconst_value_type>::value,
        "Cannot modify value referenced by GlobRef<Atomic<const T>>!");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.fetch_op()", value);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.fetch_op", _gptr);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.fetch_op", typeid(value).name());
    nonconst_value_type res;
    dart_ret_t          ret = dart_fetch_and_op(
        _gptr,
        reinterpret_cast<const void *const>(&value),
        reinterpret_cast<void *const>(&res),
        dash::dart_punned_datatype<T>::value,
        binary_op.dart_operation());
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_fetch_op failed");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.fetch_op >", res);
    return res;
  }

  /**
   * Atomically exchanges value
   */
  T exchange(const T &value) const
  {
    return fetch_op(dash::second<T>(), value);
  }

  /**
   * Atomically compares the value with the value of expected and if thosei
   * are bitwise-equal, replaces the former with desired.
   *
   * \return  True if value is exchanged
   *
   * \see \c dash::atomic::compare_exchange
   */
  bool compare_exchange(const T &expected, const T &desired) const
  {
    static_assert(
        std::is_same<value_type, nonconst_value_type>::value,
        "Cannot modify value referenced by GlobRef<const T>!");
    DASH_LOG_DEBUG_VAR("GlobRef<Atomic>.compare_exchange()", desired);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.compare_exchange", _gptr);
    DASH_LOG_TRACE_VAR("GlobRef<Atomic>.compare_exchange", expected);
    DASH_LOG_TRACE_VAR(
        "GlobRef<Atomic>.compare_exchange", typeid(desired).name());
    nonconst_value_type result;
    dart_ret_t          ret = dart_compare_and_swap(
        _gptr,
        reinterpret_cast<const void *const>(&desired),
        reinterpret_cast<const void *const>(&expected),
        reinterpret_cast<void *const>(&result),
        dash::dart_punned_datatype<T>::value);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_compare_and_swap failed");
    DASH_LOG_DEBUG_VAR(
        "GlobRef<Atomic>.compare_exchange >", (expected == result));
    return (expected == result);
  }

  /**
   * DASH specific variant which is faster than \c fetch_add
   * but does not return value
   */
  void add(const T &value) const
  {
    static_assert(
        std::is_same<value_type, nonconst_value_type>::value,
        "Cannot modify value referenced by GlobRef<const T>!");
    op(dash::plus<T>(), value);
  }

  /**
   * Atomic fetch-and-add operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  T fetch_add(
      /// Value to be added to global atomic variable.
      const T &value) const
  {
    return fetch_op(dash::plus<T>(), value);
  }

  /**
   * DASH specific variant which is faster than \c fetch_sub
   * but does not return value
   */
  void sub(const T &value) const
  {
    op(dash::plus<T>(), -value);
  }

  /**
   * Atomic fetch-and-sub operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  T fetch_sub(
      /// Value to be subtracted from global atomic variable.
      const T &value) const
  {
    return fetch_op(dash::plus<T>(), -value);
  }

  /**
   * DASH specific variant which is faster than \c fetch_multiply
   * but does not return value
   */
  void multiply(const T &value) const
  {
    op(dash::multiply<T>(), value);
  }

  /**
   * Atomic fetch-and-multiply operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  T fetch_multiply(
      /// Value to be added to global atomic variable.
      const T &value) const
  {
    return fetch_op(dash::multiply<T>(), value);
  }

  /**
   * prefix atomically increment value by one
   *
   * \return  The value of the referenced shared variable after the
   *          operation.
   *
   * \note This operator does not return a reference but a copy of the value
   * in order to ensure atomicity. This is consistent with the C++ std::atomic
   * \c operator++, see
   * http://en.cppreference.com/w/cpp/atomic/atomic/operator_arith.
   */
  T operator++() const
  {
    return fetch_add(1) + 1;
  }

  /**
   * postfix atomically increment value by one
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  T operator++(int)const
  {
    return fetch_add(1);
  }

  /**
   * prefix atomically decrement value by one
   *
   * \return  The value of the referenced shared variable after the
   *          operation.
   *
   * \note This operator does not return a reference but a copy of the value
   * in order to ensure atomicity. This is consistent with the C++ std::atomic
   * \c operator--, see
   * http://en.cppreference.com/w/cpp/atomic/atomic/operator_arith.
   */
  T operator--() const
  {
    return fetch_sub(1) - 1;
  }

  /**
   * postfix atomically decrement value by one
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  T operator--(int)const
  {
    return fetch_sub(1);
  }

  /**
   * atomically increment value by ref
   *
   * \return  The value of the referenced shared variable after the
   *          operation.
   *
   * \note This operator does not return a reference but a copy of the value
   * in order to ensure atomicity. This is consistent with the C++ std::atomic
   * \c operator+=, see
   * http://en.cppreference.com/w/cpp/atomic/atomic/operator_arith2.
   */
  T operator+=(const T &value) const
  {
    return fetch_add(value) + value;
  }

  /**
   * atomically decrement value by ref
   *
   * \return  The value of the referenced shared variable after the
   *          operation.
   *
   * Note that this operator does not return a reference but a copy of the
   * value in order to ensure atomicity. This is consistent with the C++
   * std::atomic \c operator-=, see
   * http://en.cppreference.com/w/cpp/atomic/atomic/operator_arith2.
   */
  T operator-=(const T &value) const
  {
    return fetch_sub(value) - value;
  }
};

}  // namespace dash

#endif  // DASH__ATOMIC_GLOBREF_H_

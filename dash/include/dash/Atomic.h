#ifndef DASH__ATOMIC_H__INCLUDED
#define DASH__ATOMIC_H__INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/Exception.h>
#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>
#include <dash/algorithm/Operation.h>

#include <dash/internal/Logging.h>

namespace dash {

template<typename ValueType>
class Atomic
{
  typedef Atomic<ValueType>                              self_t;

public:
  typedef ValueType                                  value_type;
  typedef size_t                                      size_type;
  typedef size_t                                difference_type;

  typedef       GlobRef<value_type>                   reference;
  typedef const GlobRef<value_type>             const_reference;

  typedef       GlobPtr<value_type>                     pointer;
  typedef const GlobPtr<value_type>               const_pointer;

public:
  /**
   * Default constructor.
   * Sets underlying global pointer to \c DART_GPTR_NULL.
   */
  Atomic()
  : _gptr(DART_GPTR_NULL),
    _team(nullptr)
  { }

  /**
   * Constructor, creates a new instance of \c dash::Atomic from a DART
   * pointer.
   */
  Atomic(
    dart_gptr_t  gptr,
    dash::Team & team = dash::Team::All())
  : _gptr(gptr),
    _team(&team)
  { }

  /**
   * Constructor, creates a new instance of \c dash::Atomic from any object
   * that is convertible to \c dart_gptr_t.
   */
  template<class GlobalType>
  Atomic(
    const GlobalType & global,
    dash::Team       & team   = dash::Team::All())
  : Atomic(global.dart_gptr(), team)
  { }

  Atomic(const self_t & other)           = default;

  self_t & operator=(const self_t & rhs) = default;

  void flush_local() const
  {
    dart_flush_local(_gptr);
  }

  void flush() const
  {
    dart_flush(_gptr);
  }

  /**
   * Set the value of the shared atomic variable.
   */
  void set(ValueType val)
  {
    DASH_LOG_DEBUG_VAR("Atomic.set()", val);
    DASH_LOG_TRACE_VAR("Atomic.set",   _gptr);
    *(reference(_gptr)) = val;
    DASH_LOG_DEBUG("Atomic.set >");
  }

  /**
   * Get a reference on the shared atomic value.
   */
  reference get()
  {
    DASH_LOG_DEBUG("Atomic.get()");
    DASH_LOG_TRACE_VAR("Atomic.get", _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    reference ref(_gptr);
    DASH_LOG_DEBUG_VAR("Atomic.get >", static_cast<ValueType>(ref));
    return ref;
  }

  /**
   * Get a const reference on the shared atomic value.
   */
  const_reference get() const
  {
    DASH_LOG_DEBUG("Atomic.cget()");
    DASH_LOG_TRACE_VAR("Atomic.cget", _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    const_reference cref(_gptr);
    DASH_LOG_DEBUG_VAR("Atomic.cget >", static_cast<ValueType>(cref));
    return cref;
  }

  /**
   * Atomically executes specified operation on the referenced shared value.
   */
  template<typename BinaryOp>
  void op(
    BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    ValueType value)
  {
    DASH_LOG_DEBUG_VAR("Atomic.add()", value);
    DASH_LOG_TRACE_VAR("Atomic.add",   _gptr);
    DASH_LOG_TRACE_VAR("Atomic.add",   _team);
    DASH_ASSERT(_team != nullptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type acc = value;
    DASH_LOG_TRACE("Atomic.add", "dart_accumulate");
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<char *>(&acc),
                       1,
                       dash::dart_datatype<ValueType>::value,
                       binary_op.dart_operation(),
                       _team->dart_id());
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_TRACE("Atomic.add", "flush");
//  dart_flush(_gptr);
//  dart_flush_local(_gptr);
    DASH_LOG_DEBUG_VAR("Atomic.add >", acc);
  }

  /**
   * Atomic fetch-and-op operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<typename BinaryOp>
  ValueType fetch_and_op(
    BinaryOp  op,
    /// Value to be added to global atomic variable.
    ValueType val)
  {
    DASH_LOG_DEBUG_VAR("Atomic.fetch_and_op()", val);
    DASH_LOG_TRACE_VAR("Atomic.fetch_and_op",   _gptr);
    DASH_ASSERT(_team != nullptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type acc;
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<void *>(&val),
                       reinterpret_cast<void *>(&acc),
                       dash::dart_datatype<ValueType>::value,
                       op.dart_operation(),
                       _team->dart_id());
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_TRACE("Atomic.fetch_and_op", "flush");
//  dart_flush(_gptr);
//  dart_flush_local(_gptr);
    DASH_LOG_DEBUG_VAR("Atomic.fetch_and_op >", acc);
    return acc;
  }

  /**
   * Atomic add operation on the referenced shared value.
   */
  void add(
    ValueType value)
  {
    op(dash::plus<ValueType>(), value);
  }

  /**
   * Atomic subtract operation on the referenced shared value.
   */
  void sub(
    ValueType value)
  {
    op(dash::plus<ValueType>(), -value);
  }

  /**
   * Atomic fetch-and-add operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  ValueType fetch_and_add(
    /// Value to be added to global atomic variable.
    ValueType val)
  {
    return fetch_and_op(dash::plus<ValueType>(), val);
  }

  /**
   * Atomic fetch-and-sub operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  ValueType fetch_and_sub(
    /// Value to be subtracted from global atomic variable.
    ValueType val)
  {
    return fetch_and_op(dash::plus<ValueType>(), -val);
  }

private:
  /// The atomic value's underlying global pointer.
  dart_gptr_t   _gptr = DART_GPTR_NULL;
  /// Team of units associated with the shared atomic variable.
  dash::Team  * _team = nullptr;

}; // class Atomic

} // namespace dash

#endif // DASH__ATOMIC_H__INCLUDED

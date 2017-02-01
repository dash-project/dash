#ifndef DASH__ATOMIC__ATOMICADDRESS_H__INCLUDED
#define DASH__ATOMIC__ATOMICADDRESS_H__INCLUDED

#include <type_traits>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/Exception.h>
#include <dash/GlobPtr.h>
#include <dash/algorithm/Operation.h>

#include <dash/internal/Logging.h>

namespace dash {

// forward decls
template<typename T>
class GlobRef;

template<typename ValueType>
class AtomicAddress 
{

  static_assert(dash::dart_datatype<ValueType>::value != DART_TYPE_UNDEFINED,
                  "dash::AtomicAddress only valid on integral and floating point types");

  typedef AtomicAddress<ValueType>                       self_t;

public:
  typedef ValueType                                  value_type;
  typedef dash::default_size_t                        size_type;
  typedef dash::default_index_t                 difference_type;

  typedef       GlobRef<value_type>                   reference;
  typedef const GlobRef<value_type>             const_reference;

  typedef       GlobPtr<value_type>                     pointer;
  typedef const GlobPtr<value_type>               const_pointer;

public:
  /**
   * Default constructor.
   * Sets underlying global pointer to \c DART_GPTR_NULL.
   */
  AtomicAddress() = default;

  /**
   * Constructor, creates a new instance of \c dash::AtomicAddress from a DART
   * pointer.
   */
  AtomicAddress(
    const dart_gptr_t & gptr,
    dash::Team & team = dash::Team::All())
  : _gptr(gptr),
    _dart_teamid(team.dart_id())
  { }

  /**
   * Constructor, creates a new instance of \c dash::AtomicAddress from any object
   * that is convertible to \c dart_gptr_t.
   */
  template<class GlobalType>
  AtomicAddress(
    const GlobalType & global,
    dash::Team       & team   = dash::Team::All())
  : AtomicAddress(global.dart_gptr(), team)
  { }

  AtomicAddress(const self_t & other)    = default;

  /**
   * Non atomic copy assignment operator
   */
  self_t & operator=(const self_t & rhs) = default;

  /**
   * Set the value of the shared atomic variable.
   */
  void set(ValueType value)
  {
    DASH_LOG_DEBUG_VAR("AtomicAddress.set()", value);
    DASH_LOG_TRACE_VAR("AtomicAddress.set",   _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<char *>(&value),
                       1,
                       dash::dart_datatype<ValueType>::value,
                       DART_OP_REPLACE,
                       _dart_teamid);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG("AtomicAddress.set >");
  }

  /**
   * Get a reference on the shared atomic value.
   */
  ValueType get()
  {
    DASH_LOG_DEBUG("AtomicAddress.get()");
    DASH_LOG_TRACE_VAR("AtomicAddress.get", _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type nothing;
    value_type result;
    /*
     * This should be replaced by a DART version of MPI_Get_Accumulate,
     * as fetch_and_op might segfault (only here?) with MPI_NO_OP.
     * 
     * https://www.mpich.org/static/docs/v3.2/www3/MPI_Get_accumulate.html
     */
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<void *>(&nothing),
                       reinterpret_cast<void *>(&result),
                       dash::dart_datatype<ValueType>::value,
                       DART_OP_NO_OP,
                       _dart_teamid);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("AtomicAddress.get >", result);
    return result;
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
    DASH_LOG_DEBUG_VAR("AtomicAddress.add()", value);
    DASH_LOG_TRACE_VAR("AtomicAddress.add",   _gptr);
    DASH_ASSERT(_dart_teamid != dash::Team::Null().dart_id());
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type acc = value;
    DASH_LOG_TRACE("AtomicAddress.add", "dart_accumulate");
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<char *>(&acc),
                       1,
                       dash::dart_datatype<ValueType>::value,
                       binary_op.dart_operation(),
                       _dart_teamid);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_DEBUG_VAR("AtomicAddress.add >", acc);
  }

  /**
   * Atomic fetch-and-op operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<typename BinaryOp>
  ValueType fetch_and_op(
    BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    ValueType value)
  {
    DASH_LOG_DEBUG_VAR("AtomicAddress.fetch_and_op()", value);
    DASH_LOG_TRACE_VAR("AtomicAddress.fetch_and_op",   _gptr);
    DASH_LOG_TRACE_VAR("AtomicAddress.fetch_and_op",   typeid(value).name());
    DASH_ASSERT(_dart_teamid != dash::Team::Null().dart_id());
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type acc;
    dart_ret_t ret = dart_fetch_and_op(
                       _gptr,
                       reinterpret_cast<void *>(&value),
                       reinterpret_cast<void *>(&acc),
                       dash::dart_datatype<ValueType>::value,
                       binary_op.dart_operation(),
                       _dart_teamid);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_fetch_and_op failed");
    DASH_LOG_DEBUG_VAR("AtomicAddress.fetch_and_op >", acc);
    return acc;
  }

  /**
   * Atomic add operation on the referenced shared value.
   */
  void add (
    ValueType value)
  {
    op(dash::plus<ValueType>(), value);
  }

  /**
   * Atomic subtract operation on the referenced shared value.
   */
  void sub (
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
  ValueType fetch_and_add (
    /// Value to be added to global atomic variable.
    ValueType value)
  {
    return fetch_and_op(dash::plus<ValueType>(), value);
  }

  /**
   * Atomic fetch-and-sub operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  ValueType fetch_and_sub (
    /// Value to be subtracted from global atomic variable.
    ValueType value)
  {
    return fetch_and_op(dash::plus<ValueType>(), -value);
  }
  
  /**
   * Atomically fetches and replaces the value.
   * 
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  ValueType exchange(
    /// Value to be set
    ValueType value)
  {
    return fetch_and_op(dash::second<ValueType>(), value);
  }
  
  /**
   * Atomically compares the value with the value of expected and if those are
   * bitwise-equal, replaces the former with desired.
   * 
   * \return  True if value is exchanged
   */
  bool compare_exchange(
    /// value expected to be found in the atomic object
    ValueType expected,
    /// value to store in the atomic object if it is as expected
    ValueType desired)
  {
    DASH_LOG_DEBUG_VAR("AtomicAddress.compare_exchange()", desired);
    DASH_LOG_TRACE_VAR("AtomicAddress.compare_exchange",   _gptr);
    DASH_LOG_TRACE_VAR("AtomicAddress.compare_exchange",   expected);
    DASH_LOG_TRACE_VAR("AtomicAddress.compare_exchange",   typeid(desired).name());
    DASH_ASSERT(_dart_teamid != dash::Team::Null().dart_id());
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    value_type result;
    dart_ret_t ret = dart_compare_and_swap(
                       _gptr,
                       reinterpret_cast<void *>(&desired),
                       reinterpret_cast<void *>(&expected),
                       reinterpret_cast<void *>(&result),
                       dash::dart_datatype<ValueType>::value,
                       _dart_teamid);
    dart_flush(_gptr);
    DASH_ASSERT_EQ(DART_OK, ret, "dart_compare_and_swap failed");
    DASH_LOG_DEBUG_VAR("AtomicAddress.compare_exchange >", (desired == result));
    return (desired == result);
  }

private:
  /// The atomic value's underlying global pointer.
  dart_gptr_t   _gptr = DART_GPTR_NULL;
  /// Team of units associated with the shared atomic variable.
  dart_team_t   _dart_teamid = dash::Team::Null().dart_id();

}; // class AtomicAddress

} // namespace dash

#endif // DASH__ATOMIC__ATOMICADDRESS_H__INCLUDED

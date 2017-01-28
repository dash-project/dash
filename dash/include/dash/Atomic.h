#ifndef DASH__ATOMIC_H__INCLUDED
#define DASH__ATOMIC_H__INCLUDED

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
  typedef AtomicAddress<ValueType>                              self_t;

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

  AtomicAddress(const self_t & other)           = default;

  self_t & operator=(const self_t & rhs) = default;

  /**
   * Set the value of the shared atomic variable.
   */
  void set(ValueType value)
  {
    DASH_LOG_DEBUG_VAR("AtomicAddress.set()", value);
    DASH_LOG_TRACE_VAR("AtomicAddress.set",   _gptr);
    *(pointer(_gptr)) = value;
    DASH_LOG_DEBUG("AtomicAddress.set >");
  }

  /**
   * Get a reference on the shared atomic value.
   */
  reference get()
  {
    DASH_LOG_DEBUG("AtomicAddress.get()");
    DASH_LOG_TRACE_VAR("AtomicAddress.get", _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    reference ref(_gptr);
    DASH_LOG_DEBUG_VAR("AtomicAddress.get >", static_cast<ValueType>(ref));
    return ref;
  }

  /**
   * Get a const reference on the shared atomic value.
   */
  const_reference get() const
  {
    DASH_LOG_DEBUG("AtomicAddress.cget()");
    DASH_LOG_TRACE_VAR("AtomicAddress.cget", _gptr);
    DASH_ASSERT(!DART_GPTR_ISNULL(_gptr));
    const_reference cref(_gptr);
    DASH_LOG_DEBUG_VAR("AtomicAddress.cget >", static_cast<ValueType>(cref));
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
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    DASH_LOG_TRACE("AtomicAddress.add", "flush");
    dart_flush_all(_gptr);
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
    DASH_ASSERT_EQ(DART_OK, ret, "dart_fetch_and_op failed");
    DASH_LOG_TRACE("AtomicAddress.fetch_and_op", "flush");
    dart_flush_all(_gptr);
    DASH_LOG_DEBUG_VAR("AtomicAddress.fetch_and_op >", acc);
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
  dart_team_t   _dart_teamid = dash::Team::Null().dart_id();

}; // class AtomicAddress

/**
 * Type wrapper to mark any trivial type atomic.
 *
 * If one unit writes to an atomic object while another unit reads from it,
 * the behavior is well-defined. The DASH version follows as closely as
 * possible the interface of \cstd::atomic
 * However as data has to be transferred between
 * units using DART, the atomicity guarantees are set by the DART
 * implementation.
 *
 * \cdash::Atomic<T> objects have to be placed in a DASH container.
 *
 * \code
 *   dash::Array<dash::Atomic<int>> array(100);
 *   supported as Atomic<value_t>(value_t T) is available
 *    dash::fill(array.begin(), array.end(), 0);
 *
 *   if(dash::myid() == 0){
 *     array[10].store(5);
 *   }
 *   dash::barrier();
 *   array[10].add(1);
 *   // postcondition:
 *   // array[10] == dash::size() + 5
 * \endcode
 */
template<typename T>
class Atomic {
public:
  typedef T value_type;

  Atomic(const Atomic<T> & other) = delete;

  /**
   * Initializes the underlying value with desired.
   * The initialization is not atomic
   */
  Atomic(T value):
  _value(value){ }

  /// disabled as this violates the atomic semantics
  T operator= (T value) = delete;

  operator T() const {
    return _value;
  }
private:
  T _value;
}; // class Atomic

/**
 * type traits for \cdash::Atomic<T>
 *
 * true if type is atomic
 * false otherwise
 */
template<typename T>
struct is_atomic {
  static constexpr bool value = false;
};
template<typename T>
struct is_atomic<dash::Atomic<T>> {
  static constexpr bool value = true;
};

/**
 * Routines to perform atomic operations on atomics residing in the
 * global address space
 *
 * \code
 *  int N = dash::size();
 *  dash::Array<dash::Atomic<int>> array(N);
 *  dash::fill(array.begin(), array.end(), 0);
 *  // each unit adds 1 to each array position
 *  for(auto & el : array){
 *    dash::atomic::add(el, 1);
 *  }
 *  // postcondition:
 *  // array = {N,N,N,N,...}
 * \endcode
 */
namespace atomic {

  /**
   * Get a reference on the shared atomic value.
   */
  template<typename T>
  T load(const dash::GlobRef<dash::Atomic<T>> & ref){
    return static_cast<T>(dash::AtomicAddress<T>(ref).get());
  }

  /**
   * Set the value of the atomic reference 
   */
  template<typename T>
  void store(const dash::GlobRef<dash::Atomic<T>> & ref,
             const T value)
  {
    dash::AtomicAddress<T>(ref).set(value);
  }

  /*
   * Atomically sets the value of the atomic reference and returns
   * the old value
   */
  template<typename T>
  T exchange(const dash::GlobRef<dash::Atomic<T>> & ref,
                const T value)
  {
    // TODO implement this
    return;
  }

  /**
   * Atomically executes specified operation on the referenced shared value.
   */
  template<
    typename T,
    typename BinaryOp >
  void op(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    const BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    T value)
  {
    dash::AtomicAddress<T>(ref).op(binary_op, value);
  }

  /**
   * Atomic fetch-and-op operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<
    typename T,
    typename BinaryOp >
  T fetch_and_op(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    const BinaryOp  binary_op,
    /// Value to be added to global atomic variable.
    T value)
  {
    return dash::AtomicAddress<T>(ref).fetch_and_op(binary_op, value);
  }

  /**
   * Atomic add operation on the referenced shared value.
   */
  template<typename T>
  typename std::enable_if<
    std::is_integral<T>::value,
    void>::type
  add(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    const T value)
  {
    dash::AtomicAddress<T>(ref).add(value);
  }

  /**
   * Atomic subtract operation on the referenced shared value.
   */
  template<typename T>
  typename std::enable_if<
    std::is_integral<T>::value,
    void>::type
  sub(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    const T value)
  {
    dash::AtomicAddress<T>(ref).sub(value);
  }

  /**
   * Atomic fetch-and-add operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<typename T>
  typename std::enable_if<
    std::is_integral<T>::value,
    T>::type
  fetch_add(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    /// Value to be added to global atomic variable.
    const T value)
  {
    return dash::AtomicAddress<T>(ref).fetch_and_add(value);
  }

  /**
   * Atomic fetch-and-sub operation on the referenced shared value.
   *
   * \return  The value of the referenced shared variable before the
   *          operation.
   */
  template<typename T>
  typename std::enable_if<
    std::is_integral<T>::value,
    T>::type
  fetch_sub(
    const dash::GlobRef<dash::Atomic<T>> & ref,
    /// Value to be subtracted from global atomic variable.
    const T value)
  {
    return dash::AtomicAddress<T>(ref).fetch_and_sup(value);
  }

} // namespace atomic
} // namespace dash

#endif // DASH__ATOMIC_H__INCLUDED

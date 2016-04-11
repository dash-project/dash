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
    DASH_ASSERT(
      !DART_GPTR_EQUAL(
        _gptr, DART_GPTR_NULL));
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
    DASH_ASSERT(
      !DART_GPTR_EQUAL(
        _gptr, DART_GPTR_NULL));
    const_reference cref(_gptr);
    DASH_LOG_DEBUG_VAR("Atomic.cget >", static_cast<ValueType>(cref));
    return cref;
  }

  /**
   * Atomic add operation on the referenced shared value.
   */
  void add(
    /// Value to be added to global atomic variable.
    ValueType add_val)
  {
    DASH_LOG_DEBUG_VAR("Atomic.add()", add_val);
    DASH_LOG_TRACE_VAR("Atomic.add",   _gptr);
    DASH_ASSERT(_team != nullptr);
    DASH_ASSERT(
      !DART_GPTR_EQUAL(
        _gptr, DART_GPTR_NULL));
    dart_ret_t ret = dart_accumulate(
                       _gptr,
                       reinterpret_cast<char *>(&add_val),
                       1,
                       dash::dart_datatype<ValueType>::value,
                       dash::plus<ValueType>().dart_operation(),
                       _team->dart_id());
    DASH_ASSERT_EQ(DART_OK, ret, "dart_accumulate failed");
    dart_flush(_gptr);
    DASH_LOG_DEBUG("Atomic.add >");
  }

private:
  /// The atomic value's underlying global pointer.
  dart_gptr_t   _gptr = DART_GPTR_NULL;
  /// Team of units associated with the shared atomic variable.
  dash::Team  * _team = nullptr;

}; // class Atomic

} // namespace dash

#endif // DASH__ATOMIC_H__INCLUDED

#ifndef DASH__ALGORITHM__OPERATION_H__
#define DASH__ALGORITHM__OPERATION_H__

#include <dash/Types.h>
#include <dash/Meta.h>

#include <dash/dart/if/dart_types.h>

#include <functional>


/**
 * \defgroup DashReduceOperations DASH Reduce Operations
 *
 * Distributed reduce operation types.
 *
 */

namespace dash {

namespace internal {

/**
 * Base type of all reduce operations, primarily acts as a container of a
 * \c dart_operation_t.
 *
 * \ingroup  DashReduceOperations
 */
template <
  typename         ValueType,
  dart_operation_t OP,
  bool             enabled = true >
class ReduceOperation {
  static constexpr const dart_operation_t _op = OP;
public:
  typedef ValueType value_type;

public:
  constexpr typename std::enable_if< enabled, dart_operation_t >::type
  dart_operation() const {
    return _op;
  }
};

} // namespace internal

/**
 * Reduce operands to their minimum value.
 *
 * \see      dart_operation_t::DART_OP_MIN
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct min
: public internal::ReduceOperation< ValueType, DART_OP_MIN,
                                    dash::is_arithmetic<ValueType>::value > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return (lhs < rhs) ? lhs : rhs;
  }
};

/**
 * Reduce operands to their maximum value.
 *
 * \see      dart_operation_t::DART_OP_MAX
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct max
: public internal::ReduceOperation< ValueType, DART_OP_MAX,
                                    dash::is_arithmetic<ValueType>::value > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return (lhs > rhs) ? lhs : rhs;
  }
};

/**
 * Reduce operands to their sum.
 *
 * \see      dart_operation_t::DART_OP_SUM
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct plus
: public internal::ReduceOperation< ValueType, DART_OP_SUM,
                                    dash::is_arithmetic<ValueType>::value > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs + rhs;
  }
};

/**
 * Reduce operands to their product.
 *
 * \see      dart_operation_t::DART_OP_PROD
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct multiply
: public internal::ReduceOperation< ValueType, DART_OP_PROD,
                                    dash::is_arithmetic<ValueType>::value > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs * rhs;
  }
};

/**
 * Returns second operand. Used as replace reduce operation
 *
 * \see      dart_operation_t::DART_OP_REPLACE
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct second
: public internal::ReduceOperation< ValueType, DART_OP_REPLACE, true > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return rhs;
  }
};

/**
 * Reduce operands with binary AND
 *
 * \see      dart_operation_t::DART_OP_BAND
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct bit_and
: public internal::ReduceOperation< ValueType, DART_OP_BAND, true > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs & rhs;
  }
};

/**
 * Reduce operands with binary OR
 *
 * \see      dart_operation_t::DART_OP_BOR
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct bit_or
: public internal::ReduceOperation< ValueType, DART_OP_BOR, true > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs | rhs;
  }
};

/**
 * Reduce operands with binary XOR
 *
 * \see      dart_operation_t::DART_OP_BXOR
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct bit_xor
: public internal::ReduceOperation< ValueType, DART_OP_BXOR, true > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs ^ rhs;
  }
};

}  // namespace dash

#endif // DASH__ALGORITHM__OPERATION_H__

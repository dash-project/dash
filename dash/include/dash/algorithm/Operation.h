#ifndef DASH__ALGORITHM__OPERATION_H__
#define DASH__ALGORITHM__OPERATION_H__

#include <dash/dart/if/dart_types.h>
#include <functional>

/**
 * \defgroup DashReduceOperations DASH Reduce Operations
 *
 * Distributed reduce operation types.
 *
 */

namespace dash {

/**
 * Base type of all reduce operations, primarily acts as a container of a
 * \c dart_operation_t.
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
class ReduceOperation {

public:
  typedef ValueType value_type;

public:
  ReduceOperation(dart_operation_t op)
  : _op(op) {
  }

  constexpr dart_operation_t dart_operation() const {
    return _op;
  }

private:
  dart_operation_t _op;

};

/**
 * Reduce operands to their minimum value.
 *
 * \see      dart_operation_t::DART_OP_MIN
 *
 * \ingroup  DashReduceOperations
 */
template< typename ValueType >
struct min : public ReduceOperation<ValueType> {

public:
  min()
  : ReduceOperation<ValueType>(DART_OP_MIN) {
  }

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
struct max : public ReduceOperation<ValueType> {

public:
  max()
  : ReduceOperation<ValueType>(DART_OP_MAX) {
  }

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
struct plus : public ReduceOperation<ValueType> {

public:
  plus()
  : ReduceOperation<ValueType>(DART_OP_SUM) {
  }

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
struct multiply : public ReduceOperation<ValueType> {

public:
  multiply()
  : ReduceOperation<ValueType>(DART_OP_PROD) {
  }

  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs * rhs;
  }
};

}  // namespace dash

#endif // DASH__ALGORITHM__OPERATION_H__

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
 * Reduce operands to their sum.
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

}  // namespace dash

#endif // DASH__ALGORITHM__OPERATION_H__

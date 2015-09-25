#ifndef DASH__ALGORITHM__OPERATION_H__
#define DASH__ALGORITHM__OPERATION_H__

#include <dash/dart/if/dart_types.h>
#include <functional>

namespace dash {

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

template< typename ValueType >
struct plus : public ReduceOperation<ValueType> {

public:
  plus()
  : ReduceOperation<ValueType>(DART_OP_ADD) {
  }

  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return std::plus<ValueType>(lhs, rhs);
  }
};

}  // namespace dash

#endif // DASH__ALGORITHM__OPERATION_H__

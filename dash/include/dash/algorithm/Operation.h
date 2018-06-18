#ifndef DASH__ALGORITHM__OPERATION_H__
#define DASH__ALGORITHM__OPERATION_H__

#include <dash/Meta.h>
#include <dash/Types.h>

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


enum class OpKind {
  ARITHMETIC,
  BITWISE,
  NOOP
};

/**
 * Base type of all reduce operations, primarily acts as a container of a
 * \c dart_operation_t.
 *
 * \ingroup  DashReduceOperations
 */
template <
  typename         ValueType,
  dart_operation_t OP,
  OpKind           KIND,
  bool             enabled = true >
class ReduceOperation {
  static constexpr const dart_operation_t _op   = OP;
  static constexpr const OpKind           _kind = KIND;
public:
  typedef ValueType value_type;

public:
  static constexpr dart_operation_t
  dart_operation() {
    return _op;
  }

  static constexpr OpKind
  op_kind() {
    return _kind;
  }
};

} // namespace internal

#ifdef DOXYGEN
/**
 * Query the underlying \ref dart_operation_t for arbitrary binary operations.
 * Yields \c DART_OP_UNDEFINED for non-DART operations.
 *
 * \see      dash::min
 * \see      dash::max
 * \see      dash::plus
 * \see      dash::multiply
 * \see      dash::first
 * \see      dash::second
 * \see      dash::bit_and
 * \see      dash::bit_or
 * \see      dash::bit_xor
 *
 * \ingroup  DashReduceOperations
 */
template<typename BinaryOperation>
struct dart_operation
{
  dart_operation_t value;
}
#else // DOXYGEN
/**
 * Query the DART operation for an arbitrary binary operations.
 * Overload for non-DART operations.
 */
template<typename BinaryOperation, typename = void>
struct dart_reduce_operation
  : public std::integral_constant<dart_operation_t, DART_OP_UNDEFINED>
{ };

/**
 * Query the DART operation for an arbitrary binary operation.
 * Overload for DART operations.
 */
template<>
template<typename BinaryOperation>
struct dart_reduce_operation<BinaryOperation,
        typename std::enable_if<
          BinaryOperation::op_kind() != dash::internal::OpKind::NOOP &&
          std::is_base_of<
            dash::internal::ReduceOperation<
              typename BinaryOperation::value_type,
              BinaryOperation::dart_operation(),
              BinaryOperation::op_kind(), true>,
            BinaryOperation>::value>::type>
  : public std::integral_constant<dart_operation_t,
                                  BinaryOperation::dart_operation()>
{ };
#endif // DOXYGEN

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
                                    dash::internal::OpKind::ARITHMETIC,
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
                                    dash::internal::OpKind::ARITHMETIC,
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
                                    dash::internal::OpKind::ARITHMETIC,
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
                                    dash::internal::OpKind::ARITHMETIC,
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
struct first
: public internal::ReduceOperation< ValueType, DART_OP_NO_OP,
                                    dash::internal::OpKind::NOOP, true > {
  ValueType operator()(const ValueType& lhs, const ValueType& /*rhs*/) const
  {
    return lhs;
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
: public internal::ReduceOperation< ValueType, DART_OP_REPLACE,
                                    dash::internal::OpKind::NOOP, true > {
  ValueType operator()(const ValueType& /*lhs*/, const ValueType& rhs) const
  {
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
: public internal::ReduceOperation< ValueType, DART_OP_BAND,
                                    dash::internal::OpKind::BITWISE, true > {
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
: public internal::ReduceOperation< ValueType, DART_OP_BOR,
                                    dash::internal::OpKind::BITWISE, true > {
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
: public internal::ReduceOperation< ValueType, DART_OP_BXOR,
                                    dash::internal::OpKind::BITWISE, true > {
  ValueType operator()(
    const ValueType & lhs,
    const ValueType & rhs) const {
    return lhs ^ rhs;
  }
};

}  // namespace dash

#endif // DASH__ALGORITHM__OPERATION_H__

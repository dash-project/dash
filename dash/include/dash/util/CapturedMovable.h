#ifndef DASH__UTIL__CAPTURED_MOVABLE_H__INCLUDED
#define DASH__UTIL__CAPTURED_MOVABLE_H__INCLUDED

#include <utility>


namespace dash {

/**
 * Utility class template to capture values from both moved temporaries
 * (rvalues) and named references.
 *
 * Use as specializations:
 *
 * - \c CapturedMoveable<T &> to capture lvalues
 * - \c CapturedMoveable<T>   to capture rvalues
 *
 */
template <class ValueType>
class CapturedMoveable {
  ValueType _value;

public:
  CapturedMoveable(ValueType && value)
  : _value(std::forward<ValueType>(value))
  { }

  CapturedMoveable(ValueType & value)
  : _value(value)
  { }

  operator ValueType & () {
    return _value;
  }

  operator const ValueType & () const {
    return _value;
  }
};

} // namespace dash

#endif // DASH__UTIL__CAPTURED_MOVABLE_H__INCLUDED

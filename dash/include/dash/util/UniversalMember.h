#ifndef DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED
#define DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

#include <utility>


namespace dash {

/**
 * Utility class template to capture values from both moved temporaries
 * (rvalues) and named references.
 *
 * Use as specializations:
 *
 * - \c UniversalMember<T &> to capture lvalues
 * - \c UniversalMember<T>   to capture rvalues
 *
 */
template <class ValueType>
class UniversalMember {
  typedef UniversalMember<ValueType> self_t;

  ValueType _value;

public:
  UniversalMember() = delete;

  explicit UniversalMember(ValueType && value)
  : _value(std::forward<ValueType>(value))
  { }

  UniversalMember(self_t && other)
  : _value(std::forward<ValueType>(other._value))
  { }

  UniversalMember(const self_t & other) = delete;

  self_t & operator=(const self_t & rhs) = delete;

  self_t & operator=(self_t && rhs) {
    _value = std::move(rhs._value);
    return *this;
  }

  operator       ValueType & ()       { return _value; }
  operator const ValueType & () const { return _value; }

};

} // namespace dash

#endif // DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

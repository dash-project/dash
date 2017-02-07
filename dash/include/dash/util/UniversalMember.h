#ifndef DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED
#define DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

#include <utility>
#include <memory>


namespace dash {

/**
 * Utility class template to capture values from both moved temporaries
 * (rvalues) and named references to avoid temporary copies.
 *
 */
template <class ValueType>
class UniversalMember {
  typedef UniversalMember<ValueType> self_t;

  std::shared_ptr<ValueType> _value;
public:
  UniversalMember() = delete;

  UniversalMember(UniversalMember<ValueType> && other)      = default;
  UniversalMember(const UniversalMember<ValueType> & other) = default;

  explicit UniversalMember(ValueType && value)
  : _value(std::make_shared<ValueType>(std::move(value)))
  { }
  explicit UniversalMember(ValueType & value)
  : _value(&value, [](ValueType *) { /* no deleter */ })
  { }

  operator       ValueType & ()       { return *(_value.get()); }
  operator const ValueType & () const { return *(_value.get()); }

  self_t & operator=(ValueType && value) {
    *(_value.get()) = std::move(value);
    return *this;
  }

  self_t & operator=(const ValueType & value) {
    *(_value.get()) = value;
    return *this;
  }
};

} // namespace dash

#endif // DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

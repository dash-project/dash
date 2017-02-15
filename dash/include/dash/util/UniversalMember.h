#ifndef DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED
#define DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

#include <utility>
#include <memory>


namespace dash {

/**
 * Utility class template to capture values from both moved temporaries
 * (rvalues) and named references to avoid temporary copies.
 *
 * \par Usage example
 *
 * \code
 *    // Class using dash::UniversalMember 
 *    template <class T>
 *    class MyClass {
 *      dash::UniversalMember<T> _value;
 *    public:
 *      constexpr explicit MyClass(T && value)
 *      : _value(std::forward<T>(value))
 *      { }

 *      constexpr explicit MyClass(const T & value)
 *      : _value(value)
 *      { }

 *            T & value()       { return _value; }
 *      const T & value() const { return _value; }
 *    };
 *
 *    // Value type definition and passing values to a class using
 *    // dash::UniversalMember
 *    template <
 *      class T,
 *      class ValueT = typename std::remove_const<
 *                       typename std::remove_reference<T>::type
 *                     >::type
 *    >
 *    MyClass<ValueT>
 *    make_my_class(T && val) {
 *      return MyClass<ValueT>(std::forward<T>(val));
 *    }
 * \endcode
 *
 */
template <class ValueType>
class UniversalMember {
  typedef UniversalMember<ValueType> self_t;

  std::shared_ptr<ValueType> _value;
public:
  UniversalMember() = delete;

  constexpr UniversalMember(self_t && other)      = default;
  constexpr UniversalMember(const self_t & other) = default;

  self_t & operator=(const self_t & other)        = default;
  self_t & operator=(self_t && other)             = default;

  constexpr explicit UniversalMember(ValueType && value)
  : _value(std::make_shared<ValueType>(std::move(value)))
  { }
  constexpr explicit UniversalMember(const ValueType & value)
  : _value(&const_cast<ValueType &>(value),
           [](ValueType *) { /* no deleter */ })
  { }

            operator       ValueType & ()       { return *(_value.get()); }
  constexpr operator const ValueType & () const { return *(_value.get()); }

  self_t & operator=(ValueType && value) {
    *(_value.get()) = std::forward<ValueType>(value);
    return *this;
  }
};

} // namespace dash

#endif // DASH__UTIL__UNIVERSAL_MEMBER_H__INCLUDED

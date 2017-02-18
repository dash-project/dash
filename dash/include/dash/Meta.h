#ifndef DASH__META_H__INCLUDED
#define DASH__META_H__INCLUDED

#ifndef DOXYGEN

#define DASH__META__DEFINE_TRAIT__HAS_TYPE(DepType) \
  template<typename T> \
  struct has_type_##DepType { \
  private: \
    typedef char                      yes; \
    typedef struct { char array[2]; } no; \
    template<typename C> static yes test(typename C:: DepType *); \
    template<typename C> static no  test(...); \
  public: \
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes); \
  };

namespace dash {

DASH__META__DEFINE_TRAIT__HAS_TYPE(iterator);
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_iterator);
DASH__META__DEFINE_TRAIT__HAS_TYPE(reference);
DASH__META__DEFINE_TRAIT__HAS_TYPE(const_reference);
DASH__META__DEFINE_TRAIT__HAS_TYPE(value_type);

} // namespace dash

#include <type_traits>
#include <functional>

namespace dash {

/*
 * For reference, see
 *
 *   "Working Draft, C++ Extension for Ranges"
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/n4569.pdf
 *
 */

template <class I>
using reference_t = typename std::add_lvalue_reference<I>::type;

template <class I>
using rvalue_reference_t =
  decltype(std::move(std::declval<dash::reference_t<I>>()));

// void f(ValueType && val) { 
//   std::string i(move(val)); 
//   // ...
// } 
//
// ValueType val;
// std::bind(f, dash::make_adv(std::move(val)))();

template<typename T> struct adv { 
  T _value; 
  explicit adv(T && value) : _value(std::forward<T>(value)) {} 
  template<typename ...U> T && operator()(U &&...) { 
    return std::forward<T>(_value); 
  } 
}; 

template<typename T> adv<T> make_adv(T && value) { 
  return adv<T> { std::forward<T>(value) }; 
}

} // namespace dash

namespace std { 
  template<typename T> 
  struct is_bind_expression< dash::adv<T> > : std::true_type {}; 
} 

#endif // DOXYGEN

#endif // DASH__META_H__INCLUDED

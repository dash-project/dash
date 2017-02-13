#ifndef DASH__META_H__INCLUDED
#define DASH__META_H__INCLUDED

namespace dash {

#ifndef DOXYGEN

/*
 * For reference, see
 *
 *   "Working Draft, C++ Extension for Ranges"
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/n4569.pdf
 *
 */

template <class I>
using rvalue_reference_t =
  decltype(std::move(declval<reference_t<I>>()));

// void f(ValueType && val) { 
//   std::string i(move(val)); 
//   // ...
// } 
//
// ValueType val;
// std::bind(f, dash::make_adv(std::move(val)))();

template<typename T> struct adv { 
  T _value; 
  explicit adv(T && value) : _value(forward<T>(value)) {} 
  template<typename ...U> T && operator()(U &&...) { 
    return forward<T>(_value); 
  } 
}; 

template<typename T> adv<T> make_adv(T && value) { 
  return adv<T> { forward<T>(value) }; 
}

namespace std { 
  template<typename T> 
  struct is_bind_expression< adv<T> > : std::true_type {}; 
} 

#endif // DOXYGEN

} // namespace dash

#endif DASH__META_H__INCLUDED

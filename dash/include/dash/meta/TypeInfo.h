#ifndef DASH__META__TYPE_INFO_H__INCLUDED
#define DASH__META__TYPE_INFO_H__INCLUDED

#include <string>
#include <typeinfo>

namespace dash {

namespace internal {
  std::string demangle(const char * typeid_name);
}

/**
 * Returns string containing the name of the specified type.
 *
 * Similar to the `typeid` operator but ensures human-readable, demangled
 * format.
 */
template <class T>
std::string typestr() {
  typedef typename std::remove_reference<T>::type TR;
  std::string r = dash::internal::demangle(typeid(TR).name());
  if (std::is_const<TR>::value)
      r = "const " + r;
  if (std::is_volatile<TR>::value)
      r = "volatile " + r;
  if (std::is_lvalue_reference<T>::value)
      r += "&";
  else if (std::is_rvalue_reference<T>::value)
      r += "&&";
  return r;
}

/**
 * Returns string containing the type name of the given object.
 *
 * Similar to the `typeid` operator but ensures human-readable, demangled
 * format.
 */
template <class T>
std::string typestr(T && val) {
  return typestr<decltype(std::forward<T>(val))>();
}

} // namespace dash

#endif // DASH__META__TYPE_INFO_H__INCLUDED

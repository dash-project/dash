#ifndef DASH__META__TYPE_INFO_H__INCLUDED
#define DASH__META__TYPE_INFO_H__INCLUDED

#include <string>
#include <typeinfo>

namespace dash {

namespace internal {
  std::string demangle(const char * typeid_name);
}

/**
 * Returns string containing the type name of the given object.
 *
 * Similar to the `typeid` operator but ensures human-readable, demangled
 * format.
 */
template <class T>
std::string typestr(const T & obj) {
  return dash::internal::demangle(
           typeid(obj).name()
         );
}

/**
 * Returns string containing the name of the specified type.
 *
 * Similar to the `typeid` operator but ensures human-readable, demangled
 * format.
 */
template <class T>
std::string typestr() {
  return dash::internal::demangle(
           typeid(T).name()
         );
}

} // namespace dash

#endif // DASH__META__TYPE_INFO_H__INCLUDED

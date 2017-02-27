#ifndef DASH__INTERNAL__TYPE_INFO_H__INCLUDED
#define DASH__INTERNAL__TYPE_INFO_H__INCLUDED

#include <string>
#include <typeinfo>

namespace dash {
namespace internal {

std::string demangle(const char * typeid_name);

template <class T>
std::string typestr(const T & obj) {
  return dash::internal::demangle(
           typeid(obj).name()
         );
}

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__TYPE_INFO_H__INCLUDED

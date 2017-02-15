
#include <dash/internal/TypeInfo.h>

#include <dash/internal/Macro.h>
#include <dash/internal/StreamConversion.h>

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstring>

#include <unistd.h>

#ifdef __GNUG__
#  include <cxxabi.h>
#  include <cstdlib>
#  include <memory>
#endif


namespace dash {
namespace internal {

#if defined(__GNUG__) || \
    defined(HAVE_CXA_DEMANGLE)
std::string demangle(const char * typeid_name) {
  int status = -4; // to avoid compiler warning
  std::unique_ptr<char, void(*)(void*)> res {
      abi::__cxa_demangle(typeid_name, NULL, NULL, &status),
      std::free
  };
  return (status==0) ? res.get() : typeid_name ;
}
#else
std::string demangle(const char * typeid_name) {
  return typeid_name;
}
#endif

} // namespace internal
} // namespace dash


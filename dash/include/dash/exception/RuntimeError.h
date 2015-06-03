#ifndef DASH__EXCEPTION__RUNTIME_ERROR_H_
#define DASH__EXCEPTION__RUNTIME_ERROR_H_

#include <stdexcept>
#include <string>

namespace dash {
namespace exception {

class RuntimeError : ::std::runtime_error {
public:
  RuntimeError(::std::string & message)
  : std::runtime_error(message) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__RUNTIME_ERROR_H_

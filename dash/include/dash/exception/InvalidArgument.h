#ifndef DASH__EXCEPTION__INVALID_ARGUMENT_H_
#define DASH__EXCEPTION__INVALID_ARGUMENT_H_

#include <dash/exception/RuntimeError.h>
#include <string>

namespace dash {
namespace exception {

class InvalidArgument : public RuntimeError {
public:
  InvalidArgument(const ::std::string & msg)
  : RuntimeError(msg) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__INVALID_ARGUMENT_H_

#ifndef DASH__EXCEPTION__NOT_IMPLEMENTED_H_
#define DASH__EXCEPTION__NOT_IMPLEMENTED_H_

#include <stdexcept>
#include <string>

namespace dash {
namespace exception {

class NotImplemented : public ::std::runtime_error {
public:
  NotImplemented(const ::std::string & msg)
  : ::std::runtime_error(msg) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__NOT_IMPLEMENTED_H_

#ifndef DASH__EXCEPTION__ASSERTION_FAILED_H_
#define DASH__EXCEPTION__ASSERTION_FAILED_H_

#include <stdexcept>
#include <string>

namespace dash {
namespace exception {

class AssertionFailed : public ::std::runtime_error {
public:
  AssertionFailed(const ::std::string & msg)
  : ::std::runtime_error(msg) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__ASSERTION_FAILED_H_

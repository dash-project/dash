#ifndef DASH__EXCEPTION__OUT_OF_BOUNDS_H_
#define DASH__EXCEPTION__OUT_OF_BOUNDS_H_

#include <dash/exception/InvalidArgument.h>
#include <string>

namespace dash {
namespace exception {

class OutOfBounds : public dash::exception::InvalidArgument {
public:
  OutOfBounds(const ::std::string & msg)
  : InvalidArgument(msg) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__OUT_OF_BOUNDS_H_

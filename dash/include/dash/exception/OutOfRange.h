#ifndef DASH__EXCEPTION__OUT_OF_RANGE_H_
#define DASH__EXCEPTION__OUT_OF_RANGE_H_

#include <dash/exception/InvalidArgument.h>
#include <stdexcept>
#include <string>

namespace dash {
namespace exception {

class OutOfRange : public ::std::out_of_range {
public:
  OutOfRange(const ::std::string & msg)
  : ::std::out_of_range(msg) {
  }
};

} // namespace exception
} // namespace dash

#endif // DASH__EXCEPTION__OUT_OF_RANGE_H_

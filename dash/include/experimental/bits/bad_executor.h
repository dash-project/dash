#ifndef STD_EXPERIMENTAL_BITS_BAD_EXECUTOR_H
#define STD_EXPERIMENTAL_BITS_BAD_EXECUTOR_H

#include <exception>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {

class bad_executor : std::exception
{
public:
  bad_executor() noexcept {}
  ~bad_executor() noexcept {}

  virtual const char* what() const noexcept
  {
    return "bad executor";
  }
};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_BAD_EXECUTOR_H

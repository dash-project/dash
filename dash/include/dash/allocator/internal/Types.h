#ifndef DASH__ALLOCATOR__INTERNAL__TYPES_H__INCLUDED
#define DASH__ALLOCATOR__INTERNAL__TYPES_H__INCLUDED
#include <cstddef>
#include <ostream>

namespace dash {
namespace allocator {


struct memory_block {
  // Default Constructor
  memory_block() noexcept
    : ptr(nullptr)
    , length(0)
  {
  }

  memory_block(void *ptr, size_t length) noexcept
    : ptr(ptr)
    , length(length)
  {
  }

  // Move Constructor
  memory_block(memory_block &&x) noexcept { *this = std::move(x); }
  // Copy Constructor
  memory_block(const memory_block &x) noexcept = default;

  // Move Assignment
  memory_block &operator=(memory_block &&x) noexcept
  {
    ptr = x.ptr;
    length = x.length;
    x.reset();
    return *this;
  }

  // Copy Assignment
  memory_block &operator=(const memory_block &x) noexcept = default;

  /**
  * The Memory Block is not freed
  */
  ~memory_block() {}
  /**
   * Clear the memory block
   */
  void reset() noexcept
  {
    ptr = nullptr;
    length = 0;
  }

  /**
   * Bool operator to make the Allocator code better readable
   */
  explicit operator bool() const noexcept
  {
    return length != 0 && ptr != nullptr;
  }

  void *ptr;
  std::size_t length;
};

bool operator==(const memory_block & lhs, const memory_block &rhs);
bool operator!=(const memory_block & lhs, const memory_block &rhs);
std::ostream & operator<<(std::ostream &stream, const memory_block & block);
}  // namespace allocator
}  // namespace dash
#endif

#ifndef DASH__MEMORY__MEMORY_SPACE_H__INCLUDED
#define DASH__MEMORY__MEMORY_SPACE_H__INCLUDED

#include <algorithm>
#include <ostream>

namespace dash {
namespace memory {
namespace internal {

struct memory_block {
  // friend std::ostream& operator<<(std::ostream& stream, struct memory_block
  // const& block);
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

  bool operator==(const memory_block &rhs) const noexcept
  {
    return static_cast<void *>(ptr) == static_cast<void *>(rhs.ptr) && length == rhs.length;
  }

  bool operator!=(const memory_block &rhs) const noexcept
  {
    return !(*this == rhs);
  }

  std::ostream & operator<<(std::ostream & stream) const noexcept
  {
    stream << "memory_block { ptr: " << ptr
           << ", length: " << length << "}";
    return stream;
  }

  void * ptr;
  std::size_t length;
};
}  // namespace internal
}  // namespace memory
}  // namespace dash
#endif  // MemorySpace.h

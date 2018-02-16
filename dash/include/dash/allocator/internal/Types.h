#ifndef DASH__ALLOCATOR__INTERNAL__TYPES_H__INCLUDED
#define DASH__ALLOCATOR__INTERNAL__TYPES_H__INCLUDED
#include <dash/dart/if/dart_globmem.h>
#include <cstddef>
#include <tuple>
#include <ostream>

namespace dash {
namespace allocator {

template <typename LPtr, typename GPtr = dart_gptr_t>
struct allocation_rec {
  // Default Constructor
  constexpr allocation_rec() noexcept
    : _data(std::make_tuple(LPtr{}, 0, DART_GPTR_NULL))
  {
  }

  constexpr allocation_rec(LPtr ptr, size_t length) noexcept
    : _data(std::make_tuple(ptr, length, DART_GPTR_NULL))
  {
  }

  constexpr allocation_rec(LPtr ptr, size_t length, GPtr gptr) noexcept
    : _data(std::make_tuple(ptr, length, gptr))
  {
  }

  // Move Constructor
  allocation_rec(allocation_rec &&x) noexcept
    : _data(std::move(x._data))
  {
    x.reset();
  }

  // Copy Constructor
  allocation_rec(const allocation_rec &x) noexcept = default;

  // Move Assignment
  allocation_rec &operator=(allocation_rec &&x) noexcept
  {
    std::swap(_data, x._data);
    x.reset();
    return *this;
  }

  // Copy Assignment
  allocation_rec &operator=(const allocation_rec &x) noexcept = default;

  /**
   * The Memory Block is not freed
   */
  ~allocation_rec()
  {
  }
  /**
   * Clear the memory block
   */
  void reset() noexcept
  {
    std::get<0>(_data) = LPtr{};
    std::get<1>(_data) = 0;
    std::get<2>(_data) = DART_GPTR_NULL;
  }

  LPtr &lptr() noexcept
  {
    return std::get<0>(_data);
  }

  LPtr const &lptr() const noexcept
  {
    return std::get<0>(_data);
  }

  std::size_t &length() noexcept
  {
    return std::get<1>(_data);
  }

  std::size_t const &length() const noexcept
  {
    return std::get<1>(_data);
  }

  GPtr &gptr() noexcept
  {
    return std::get<2>(_data);
  }

  GPtr const &gptr() const noexcept
  {
    return std::get<2>(_data);
  }
  /**
   * Bool operator to make the Allocator code better readable
   */
  explicit operator bool() const noexcept
  {
    return (length() > 0 && lptr() != LPtr{nullptr}) ||
           !DART_GPTR_ISNULL(gptr());
  }

private:
  std::tuple<LPtr, std::size_t, GPtr> _data;
};

template <typename LPtr, typename GPtr = dart_gptr_t>
bool operator==(
    const allocation_rec<LPtr, GPtr> &lhs,
    const allocation_rec<LPtr, GPtr> &rhs)
{
  return lhs._data == rhs._data;
}

template <typename LPtr, typename GPtr = dart_gptr_t>
bool operator!=(
    const allocation_rec<LPtr, GPtr> &lhs,
    const allocation_rec<LPtr, GPtr> &rhs)
{
  return !(lhs == rhs);
}

template <typename LPtr, typename GPtr = dart_gptr_t>
std::ostream &operator<<(
    std::ostream &stream, const allocation_rec<LPtr, GPtr> &block)
{
  stream << "{lptr: " << block.ptr() << ", length: " << block.length()
         << ", gptr: " << block.gptr() << "}";
  return stream;
}

}  // namespace allocator
}  // namespace dash
#endif

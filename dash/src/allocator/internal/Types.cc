#include <dash/allocator/internal/Types.h>

namespace dash {
namespace allocator {

bool operator==(const memory_block &lhs, const memory_block &rhs)
{
  return lhs.ptr == rhs.ptr && lhs.length == rhs.length;
}
bool operator!=(const memory_block &lhs, const memory_block &rhs)
{
  return !(lhs == rhs);
}
std::ostream &operator<<(std::ostream &stream, const memory_block &block)
{
  stream << "{ptr: " << block.ptr << ", length: " << block.length << "}";
  return stream;
}
}
}

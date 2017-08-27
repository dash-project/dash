#ifndef DASH__MEMORY__UTIL_H__INCLUDED
#define DASH__MEMORY__UTIL_H__INCLUDED

#include <climits>
#include <type_traits>

namespace dash {
namespace memory {
namespace internal {


template <typename UnsignedT>
UnsignedT next_power_of_2(UnsignedT v) {
  //This loop should be unrolled since the compiler knows how many iterations we need.
  //Based on the famous Bit Twiddling Hacks
  //
  static_assert(std::is_unsigned<UnsignedT>::value, "Must be unsigned");

  v--;

  for (size_t i = 1; i < sizeof(v) * CHAR_BIT; i *= 2) {
    v |= v >> i;
  }

  return ++v;
}
} //namespace internal
} //namespace memory
} //namespace dash
#endif //DASH__MEMORY__UTIL_H__INCLUDED

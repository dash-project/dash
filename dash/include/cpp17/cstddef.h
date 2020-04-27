#ifndef INCLUDED_CSTDDEF_DOT_H
#define INCLUDED_CSTDDEF_DOT_H

#include <cstddef>

#if __cpp_lib_byte < 201603

namespace std {
// The `byte` type is defined exactly this way in C++17's `<cstddef>` (section
// [cstddef.syn]).  It is defined here to allow use of
// `polymorphic_allocator<byte>` as a vocabulary type.
enum class byte : unsigned char {};

}  // namespace std

#endif

#endif

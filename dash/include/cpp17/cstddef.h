#ifndef INCLUDED_CSTDDEF_DOT_H
#define INCLUDED_CSTDDEF_DOT_H

namespace cpp17 {
// The `byte` type is defined exactly this way in C++17's `<cstddef>` (section
// [cstddef.syn]).  It is defined here to allow use of
// `polymorphic_allocator<byte>` as a vocabulary type.
enum class byte : unsigned char {};

} // namespace cpp17

#endif


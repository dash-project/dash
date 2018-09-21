#ifndef DASH__MEMORY__RAW_DART_POINTER_H__INCLUDED
#define DASH__MEMORY__RAW_DART_POINTER_H__INCLUDED

#include <dash/Types.h>
#include <dash/dart/if/dart_globmem.h>

namespace dash {

class RawDartPointer {
  dart_gptr_t m_dart_gptr = DART_GPTR_NULL;

  using segid_t  = int16_t;
  using flags_t  = unsigned int;
  using offset_t = uint64_t;

public:
  constexpr RawDartPointer() = default;
  constexpr RawDartPointer(dart_gptr_t gptr)
    : m_dart_gptr(gptr)
  {
  }
  constexpr RawDartPointer(const RawDartPointer &other) = default;
  constexpr RawDartPointer(RawDartPointer &&)           = default;
  RawDartPointer &operator=(RawDartPointer const &) = default;
  RawDartPointer &operator=(RawDartPointer &&) = default;

  /* TeamId and SegmentId are read-only */

  constexpr dart_team_t teamid() const noexcept
  {
    return m_dart_gptr.teamid;
  }

  constexpr segid_t segid() const noexcept
  {
    return m_dart_gptr.segid;
  }

  constexpr flags_t flags() const noexcept
  {
    return m_dart_gptr.flags;
  }

  void flags(flags_t flags) noexcept
  {
    m_dart_gptr.flags = flags;
  }

  constexpr dash::team_unit_t unitid() const noexcept
  {
    return dash::team_unit_t{m_dart_gptr.unitid};
  }

  void unitid(dash::team_unit_t unitid) noexcept
  {
    m_dart_gptr.unitid = unitid;
  }

  constexpr offset_t offset() const noexcept
  {
    return m_dart_gptr.addr_or_offs.offset;
  }

  void offset(offset_t offset) noexcept
  {
    m_dart_gptr.addr_or_offs.offset = offset;
  }

  void inc_offset(std::ptrdiff_t nbytes) noexcept
  {
    if (nbytes < 0) {
      decrement(-nbytes);
    }
    else {
      increment(nbytes);
    }
  }

  void dec_offset(std::ptrdiff_t nbytes) noexcept
  {
    if (nbytes < 0) {
      increment(-nbytes);
    }
    else {
      decrement(nbytes);
    }
  }

  void * local()
  {
    void *addr = 0;
    if (dart_gptr_getaddr(m_dart_gptr, &addr) == DART_OK) {
      return addr;
    }
    return nullptr;
  }

  constexpr explicit operator bool() const noexcept
  {
    return !DART_GPTR_ISNULL(m_dart_gptr);
  }

  constexpr operator dart_gptr_t() const noexcept
  {
    return m_dart_gptr;
  }

  constexpr bool operator==(RawDartPointer const &other) const noexcept
  {
    return DART_GPTR_EQUAL(m_dart_gptr, other.m_dart_gptr);
  }
  constexpr bool operator!=(RawDartPointer const &other) const noexcept
  {
    return !(*this == other);
  }

private:
  void decrement(size_t nbytes)
  {
    m_dart_gptr.addr_or_offs.offset -= nbytes * sizeof(uint8_t);
  }

  void increment(size_t nbytes)
  {
    m_dart_gptr.addr_or_offs.offset += nbytes * sizeof(uint8_t);
  }
};

}  // namespace dash

#endif

#include <dash/GlobPtr.h>
#include <dash/memory/RawDartPointer.h>

std::ostream & operator<<(
  std::ostream      & os,
  const dart_gptr_t & dartptr)
{
  char buf[100];
  sprintf(buf,
          "<%06X|%02X|%04X|%04X|%016lX>",
          dartptr.unitid,
          dartptr.flags,
          dartptr.segid,
          dartptr.teamid,
          dartptr.addr_or_offs.offset);
  os << "dart_gptr_t(" << buf << ")";
  return os;
}

bool operator==(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs)
{
  return DART_GPTR_EQUAL(lhs, rhs);
}

bool operator!=(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs)
{
  return !DART_GPTR_EQUAL(lhs, rhs);
}

#include <dash/GlobPtr.h>

std::ostream & operator<<(
  std::ostream & os,
  const dart_gptr_t & dartptr) {
  char buf[100];
  sprintf(buf,
          "<%08X|%04X|%04X|%016lX>",
          dartptr.unitid,
          dartptr.segid,
          dartptr.flags,
          dartptr.addr_or_offs.offset);
  os << "dart_gptr_t(" << buf << ")";
  return os;
}

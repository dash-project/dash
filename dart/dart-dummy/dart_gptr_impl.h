#ifndef DART_GPTR_IMPL_H_INCLUDED
#define DART_GPTR_IMPL_H_INCLUDED

#include "dart_gptr.h"

#ifdef DART_GPTR_GETADDR
#undef DART_GPTR_GETADDR
#define DART_GPTR_GETADDR(gptr_) ((void*)gptr_.addr_or_offs.addr)
#endif


#ifdef DART_GPTR_SETADDR
#undef DART_GPTR_SETADDR
#define DART_GPTR_SETADDR(gptr_, addr_) \
  gptr_.addr_or_offs.addr=addr_;
#endif



#endif /* DART_GPTR_IMPL_H_INCLUDED */

#ifndef DART__MPI__DART_GLOBMEM_PRIV_H__
#define DART__MPI__DART_GLOBMEM_PRIV_H__

#include <stdint.h>

#define DART_GPTR_COPY(gptr_, gptrt_)                       \
  do {                                                      \
    gptr_.unitid = gptrt_.unitid;                           \
    gptr_.segid  = gptrt_.segid;                            \
    gptr_.flags  = gptrt_.flags;                            \
    gptr_.addr_or_offs.offset = gptrt_.addr_or_offs.offset; \
  } while(0)

#endif /* DART__MPI__DART_GLOBMEM_PRIV_H__ */

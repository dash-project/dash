#ifndef DART_GLOBMEM_PRIV_H
#define DART_GLOBMEM_PRIV_H
#include <stdint.h>

#define DART_GPTR_COPY(gptr_, gptrt_)                           \
    ({gptr_.addr_or_offs.offset = gptrt_.addr_or_offs.offset;   \
    gptr_.flags = gptrt_.flags;                 \
    gptr_.segid = gptrt_.segid;                 \
    gptr_.unitid = gptrt_.unitid;})

#endif /* DART_GLOBMEM_PRIV_H */

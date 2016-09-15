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


/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win dart_win_local_alloc;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
extern MPI_Win dart_sharedmem_win_local_alloc;
#endif

#endif /* DART__MPI__DART_GLOBMEM_PRIV_H__ */

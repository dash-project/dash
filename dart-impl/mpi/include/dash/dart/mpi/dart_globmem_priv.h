#ifndef DART__MPI__DART_GLOBMEM_PRIV_H__
#define DART__MPI__DART_GLOBMEM_PRIV_H__

#include <stdint.h>

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win dart_win_local_alloc;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
extern MPI_Win dart_sharedmem_win_local_alloc;
#endif

#endif /* DART__MPI__DART_GLOBMEM_PRIV_H__ */

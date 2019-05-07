#ifndef DART__MPI__DART_GLOBMEM_PRIV_H__
#define DART__MPI__DART_GLOBMEM_PRIV_H__

#include <dash/dart/base/macro.h>
#include <mpi.h>

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win dart_win_local_alloc DART_INTERNAL;

/**
 * Check that the window support MPI_WIN_UNIFIED, print warning otherwise.
 * Store the result in \c segment->sync_needed.
 */
void dart__mpi__check_memory_model(dart_segment_info_t *segment) DART_INTERNAL;

#endif /* DART__MPI__DART_GLOBMEM_PRIV_H__ */

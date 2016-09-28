/**
 * \file dash/dart/mpi/dart_locality_priv.h
 *
 * Internal implementations for the locality function component of the
 * DART-MPI library.
 */
#ifndef DART__MPI__DART_PMEM_PRIV_H
#define DART__MPI__DART_PMEM_PRIV_H

#include <dash/dart/if/dart_types.h>

dart_ret_t dart__mpi__pmem_init(void);
dart_ret_t dart__mpi__pmem_finalize(void);

#endif

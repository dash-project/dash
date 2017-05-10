/**
 * \file dash/dart/mpi/dart_locality_priv.h
 *
 * Internal implementations for the locality function component of the
 * DART-MPI library.
 */
#ifndef DART__MPI__DART_LOCALITY_PRIV_H__
#define DART__MPI__DART_LOCALITY_PRIV_H__

#include <dash/dart/if/dart_types.h>
#include <dash/dart/base/macro.h>


dart_ret_t dart__mpi__locality_init() DART_INTERNAL;

dart_ret_t dart__mpi__locality_finalize() DART_INTERNAL;

#endif /* DART__MPI__DART_LOCALITY_PRIV_H__ */

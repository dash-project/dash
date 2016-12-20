/**
 * \file dart_synchronization_priv.h
 *
 * Definition of dart_lock_struct.
 */
#ifndef DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED
#define DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_synchronization.h>

#include <stdio.h>
#include <mpi.h>

/**
 * Dart lock type.
 */
struct dart_lock_struct
{
  /** Pointer to the tail of lock queue. Stored in unit 0 by default. */
	dart_gptr_t gptr_tail;
  /** Pointer to next waiting unit, realizes distributed list across team. */
	dart_gptr_t gptr_list;
	dart_team_t teamid;
  /** Whether certain unit has acquired the lock. */
	int32_t is_acquired;
};

#endif /* DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED */

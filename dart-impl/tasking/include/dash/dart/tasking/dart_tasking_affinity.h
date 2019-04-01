#ifndef DART_TASKING_AFFINITY_H_
#define DART_TASKING_AFFINITY_H_

#include <dash/dart/base/macro.h>
#include <pthread.h>

/**
 * Initialize the affinity state.
 */
void
dart__tasking__affinity_init() DART_INTERNAL;

/**
 * Finalize affinity.
 */
void
dart__tasking__affinity_fini() DART_INTERNAL;

/**
 * Set the affinity for thread \c pthread with ID \c dart_thread_id.
 *
 * \return The core the thread was bound to.
 */
int
dart__tasking__affinity_set(pthread_t pthread, int dart_thread_id) DART_INTERNAL;

int
dart__tasking__affinity_num_numa_nodes() DART_INTERNAL;

uint32_t
dart__tasking__affinity_num_cores() DART_INTERNAL;

int
dart__tasking__affinity_core_numa_node(int core_id) DART_INTERNAL;

int
dart__tasking__affinity_ptr_numa_node(const void *ptr) DART_INTERNAL;

/**
 * Set affinity for a utility thread. This is typically done by preventing
 * the utility thread from running on the main thread's CPU.
 */
void
dart__tasking__affinity_set_utility(
  pthread_t pthread,
  int       dart_thread_id) DART_INTERNAL;

#endif // DART_TASKING_AFFINITY_H_

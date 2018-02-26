#ifndef DART_TASKING_AFFINITY_H_
#define DART_TASKING_AFFINITY_H_

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
 */
void
dart__tasking__affinity_set(pthread_t pthread, int dart_thread_id) DART_INTERNAL;

#endif // DART_TASKING_AFFINITY_H_

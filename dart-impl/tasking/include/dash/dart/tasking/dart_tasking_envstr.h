#ifndef DART__BASE__INTERNAL__TASKING_ENVSTR_H__
#define DART__BASE__INTERNAL__TASKING_ENVSTR_H__


/**
 * Name of the environment variable specifying the number of threads to use.
 *
 * Type: Positive integral value.
 */
#define DART_NUMTHREADS_ENVSTR         "DART_NUM_THREADS"

/**
 * Name of the environment variable specifying the size of the task stack size.
 *
 * Type: integral value with optional B, K, M, G qualifier.
 */
#define DART_TASKSTACKSIZE_ENVSTR      "DART_TASK_STACKSIZE"

/**
 * Name of the environment variable specifying whether threads should be
 * bound to a single CPU core/hw-thread.
 *
 * Type: Boolean value.
 */
#define DART_THREAD_AFFINITY_ENVSTR    "DART_BIND_THREADS"

/**
 * Name of the environment variable specifying the number of phases after which
 * to perform a dependency matching.
 *
 * Type: Integral positive value.
 */
#define DART_MATCHING_FREQUENCY_ENVSTR "DART_TASK_MATCHING_FREQ"

/**
 * Name of the environment variable specifying the number of phases after which
 * to perform a dependency matching.
 *
 * Type: string value, one of GET[_IMP] (default) or SENDRECV[_IMPL]
 */
#define DART_COPYIN_IMPL_ENVSTR        "DART_TASK_COPYIN_IMPL"


#endif // DART__BASE__INTERNAL__TASKING_ENVSTR_H__

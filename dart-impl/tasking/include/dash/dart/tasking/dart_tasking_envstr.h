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
 * Name of the environment variable specifying whether or not to print the
 * thread-binding.
 *
 * Type: Boolean value.
 */
#define DART_THREAD_AFFINITY_VERBOSE_ENVSTR  "DART_BIND_THREADS_VERBOSE"

/**
 * Name of the environment variable specifying the number of phases after which
 * to perform a dependency matching.
 *
 * Type: Integral positive value.
 */
#define DART_MATCHING_INTERVAL_ENVSTR "DART_TASK_MATCHING_INTERVAL"

/**
 * Name of the environment variable specifying the maximum number of phases that
 * are active, effectively throttling the task creation. The value should be a
 * multiple of the matching interval and will be rounded-up otherwise.
 *
 * Type: Integral positive value.
 * Default: 2x the matching interval.
 */
#define DART_MATCHING_PHASE_MAX_ACTIVE_ENVSTR "DART_MATCHING_PHASE_MAX_ACTIVE"

/**
 * Name of the environment variable specifying the number of phases after which
 * to perform a dependency matching.
 *
 * Type: string value, one of \c GET (default) or \c SENDRECV
 */
#define DART_COPYIN_IMPL_ENVSTR        "DART_TASK_COPYIN_IMPL"

/**
 * Name of the environment variable specifying the way the scheduler should wait
 * for a copyin operation to complete.
 *
 * Type: string value, one of \c YIELD (test-yield cycles, default) or
 *                            \c BLOCK (using \ref dart_task_wait_handle)
 */
#define DART_COPYIN_WAIT_ENVSTR        "DART_TASK_COPYIN_WAIT"

/**
 * Name of the environment variable specifying the way the scheduler should wait
 * for a copyin operation to complete.
 *
 * Type: string value, one of \c SLEEP (sleep for \c DART_TASK_IDLE_THREAD_SLEEP, default) or
 *                            \c POLL  (do not sleep) or
 *                            \c WAIT  (wait for a task to arrive)
 */
#define DART_THREAD_IDLE_ENVSTR        "DART_TASK_IDLE_THREAD"

/**
 * Name of the environment variable specifying the number of microseconds a
 * thread should sleep if no tasks are available and \c DART_TASK_IDLE_THREAD
 * is set to \c
 *
 * Type: the time in microseconds or a number postfixed with 'm'/'ms',
 *       'u'/'us', or 's'.
 */
#define DART_THREAD_IDLE_SLEEP_ENVSTR  "DART_TASK_IDLE_THREAD_SLEEP"

/**
 * Name of the environment variable specifying whether the runtime should launch
 * a progress thread to handle inter-scheduler communication.
 *
 * Type: Boolean value.
 */
#define DART_THREAD_PROGRESS_ENVSTR    "DART_THREAD_PROGRESS"

/**
 * Name of the environment variable specifying whether the runtime should launch
 * a progress thread to handle inter-scheduler communication.
 *
 * Type: Boolean value.
 */
#define DART_THREAD_PROGRESS_INTERVAL_ENVSTR    "DART_THREAD_PROGRESS_INTERVAL"

/**
 * Name of the environment variable specifying whether the runtime should
 * consider NUMA placement when scheduling tasks.
 *
 * Type: Boolean value.
 */
#define DART_THREAD_PLACE_NUMA_ENVSTR    "DART_TASK_SCHEDULE_NUMA"

#endif // DART__BASE__INTERNAL__TASKING_ENVSTR_H__

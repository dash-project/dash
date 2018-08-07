#ifndef DART_TASKING_INIT_H_
#define DART_TASKING_INIT_H_

#include <dash/dart/if/dart_types.h>

/**
 * Initialize some early data structures, i.e., managing the tread-id
 */
dart_ret_t
dart_tasking_init_early() __attribute__((weak));

/**
 * Initialize the tasking environment, i.e., create the a thread-pool waiting for tasks.
 */
dart_ret_t
dart_tasking_init() __attribute__((weak));


/**
 * Finalize and release all resource acquired during initialization.
 */
dart_ret_t
dart_tasking_fini() __attribute__((weak));

#endif /* DART_TASKING_INIT_H_ */

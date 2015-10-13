#ifndef DART_GASPI_INITIALIZATION_H
#define DART_GASPI_INITIALIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dash/dart/if/dart_types.h>

#define DART_INTERFACE_ON

/*
   Initialize the DART runtime

   A correct DASH/DART program must have exactly one pair of dart_init
   / dart_exit calls.

   It is an error to call any other DART functions before dart_init()
   or after dart_exit().
*/

/**
 * blocking collective function
 *
 * possible for GASPI
 * decide between ranks which has distributed or shared memory
 * -> can be hard, because the runtime of gaspi initial a given size of
 *    processes
 * to use threads for shared memory we need a own runtime !!
 *
 * funtion:
 * -create groups with shared memory
 *   -> create subcommunicators
 * -create a window with size DART_MAX_LENGTH(1024 * 1024 * 16)
 * -create a dynamic win object for all the dart collective allocation based on MPI_COMM_WORLD
 */

dart_ret_t dart_init(int *argc, char ***argv);

/**
 * possible for GASPI
 *
 * function free all allocated memory
 * delete groups
 * finish communication library
 */
dart_ret_t dart_exit();

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GASPI_INITIALIZATION_H */

/** @file dart_adapt_initialization.h
 *  @date 25 Mar 2014
 *  @brief Function prototypes for dart init and exit operations.
 *   
 *  Dart init and exit operations just behave like the init and finalize 
 *  respectively offered in MPI.
 */
#ifndef DART_ADAPT_INITIALIZATION_H_INCLUDED
#define DART_ADAPT_INITIALIZATION_H_INCLUDED

#include "dart_deb_log.h"
#include "dart_initialization.h"
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/** @brief Initialize dart programme, prepare for all the other dart operations.
 *  
 *  None of the dart operations should be called before calling the init.
 *
 *  Init consists of a significant amount of preparatory work, which form the prerequisite
 *  for calling the following dart operations.
 */
dart_ret_t dart_adapt_init(int *argc, char ***argv);

/** @brief Exit from a dart programme.
 *
 *  None of the dart operations should be called after calling the exit.
 */
dart_ret_t dart_adapt_exit();

dart_ret_t dart_adapt_usage(char* s);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_ADAPT_INITIALIZATION_H_INCLUDED */

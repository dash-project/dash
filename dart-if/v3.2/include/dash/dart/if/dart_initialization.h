#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_types.h"

#define DART_INTERFACE_ON

/* 
   Initialize the DART runtime 
   
   A correct DASH/DART program must have exactly one pair of dart_init
   / dart_exit calls.
   
   It is an error to call any other DART functions before dart_init()
   or after dart_exit().
*/
dart_ret_t dart_init(int *argc, char ***argv);
dart_ret_t dart_exit();

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */

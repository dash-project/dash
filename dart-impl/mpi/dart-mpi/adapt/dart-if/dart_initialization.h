#ifndef DART_INITIALIZATION_H_INCLUDED
#define DART_INITIALIZATION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_types.h"

#define DART_INTERFACE_ON

dart_ret_t dart_init(int *argc, char ***argv);
dart_ret_t dart_exit();

dart_ret_t dart_usage(char* s);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_INITIALIZATION_H_INCLUDED */

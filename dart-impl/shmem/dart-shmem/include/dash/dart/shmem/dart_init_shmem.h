#ifndef DART_INIT_SHMEM_H_INCLUDED
#define DART_INIT_SHMEM_H_INCLUDED

#include <dash/dart/shmem/extern_c.h>
EXTERN_C_BEGIN

int dart_init_shmem(int *argc, char ***argv);
dart_ret_t dart_exit_shmem();

EXTERN_C_END

#endif /* DART_INIT_SHMEM_H_INCLUDED */

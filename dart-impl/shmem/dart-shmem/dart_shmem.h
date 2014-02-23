#ifndef DART_SHMEM_H_INCLUDED
#define DART_SHMEM_H_INCLUDED

#include "extern_c.h"
EXTERN_C_BEGIN

/*  number of DART arguments passed in command line */
#define NUM_DART_ARGS    4

extern int _glob_myid; 
extern int _glob_size; 

EXTERN_C_END

#endif /* DART_SHMEM_H_INCLUDED */

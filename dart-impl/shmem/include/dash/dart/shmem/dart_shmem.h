#ifndef DART_SHMEM_H_INCLUDED
#define DART_SHMEM_H_INCLUDED

#include <dash/dart/shmem/extern_c.h>

EXTERN_C_BEGIN

/*  number of DART arguments passed 
    in command line */
#define NUM_DART_ARGS    4

extern int _glob_myid; 
extern int _glob_size; 
extern int _glob_state;

#define DART_STATE_NOT_INITIALIZED  1
#define DART_STATE_INITIALIZED      2
#define DART_STATE_FINALIZED        3

#define DART_INIT_CHECK()						\
  if( _glob_state!=DART_STATE_INITIALIZED ) return DART_ERR_NOTINIT;


EXTERN_C_END

#endif /* DART_SHMEM_H_INCLUDED */

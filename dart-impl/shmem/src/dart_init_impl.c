
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/shmem/dart_shmem.h>
#include <dash/dart/shmem/dart_init_shmem.h>

bool dart_initialized();

dart_ret_t dart_init(int *argc, char ***argv)
{
  if (dart_initialized()) {
    /* Multiple subsequent calls of dart_init, ignore: */
    return DART_OK;
  }

  dart_ret_t ret;

  if( !argc || !argv ) {
    return DART_ERR_INVAL;
  }

  if( _glob_state!=DART_STATE_NOT_INITIALIZED ) {
    return DART_ERR_INVAL;
  }  

  ret =  dart_init_shmem(argc, argv);

  _glob_state = DART_STATE_INITIALIZED;
  return ret;
}

dart_ret_t dart_exit()
{
  if (!dart_initialized()) {
    /* DART not initialized or multiple calls of dart_exit, ignore: */
    return DART_OK;
  }

  dart_ret_t ret;

  if( _glob_state!=DART_STATE_INITIALIZED ) {
    return DART_ERR_INVAL;
  }  
  
  dart_barrier(DART_TEAM_ALL);
  
  ret = dart_exit_shmem();

  _glob_state = DART_STATE_FINALIZED;
  return ret;
}

bool dart_initialized()
{
  return _glob_state == DART_STATE_INITIALIZED;
}

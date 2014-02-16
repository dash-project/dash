#include "dart_initialization.h"

#include "dart_init_shmem.h"

dart_ret_t dart_init(int *argc, char ***argv)
{
  dart_init_shmem(argc, argv);
  return DART_OK;
}

dart_ret_t dart_exit()
{
  dart_ret_t ret;
  ret = dart_exit_shmem();
  return ret;
}


#include "Init.h"

void dash::init(int *argc, char ***argv)
{
  dart_init(argc,argv);
}

void dash::finalize()
{
  dart_exit();
}

int dash::myid()
{
  dart_unit_t myid;
  dart_myid(&myid);
  return myid;
}

size_t dash::size()
{
  size_t size;
  dart_size(&size);
  return size;
}


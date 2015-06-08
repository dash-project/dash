
#include <dash/Init.h>
#include <dash/Team.h>

void dash::init(int *argc, char ***argv)
{
  dart_init(argc,argv);

  dash::Team& t = dash::Team::All();
}

void dash::finalize()
{
    dash::barrier();
  //dart_exit();
}

void dash::barrier()
{
  dash::Team::All().barrier();
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


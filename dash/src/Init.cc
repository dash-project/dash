
#include <dash/Init.h>
#include <dash/Team.h>

namespace dash {
  static int  _myid        = -1;
  static bool _initialized = false;
}

void dash::init(int *argc, char ***argv)
{
  dart_init(argc,argv);

  dash::Team& t = dash::Team::All();

  dash::_initialized = true;
}

void dash::finalize()
{
  dash::barrier();
//dart_exit();
}

bool dash::is_initialized() {
  return dash::_initialized;
}

void dash::barrier()
{
  dash::Team::All().barrier();
}

int dash::myid()
{
  if (dash::_myid < 0 && dash::is_initialized()) {
    dart_unit_t myid;
    dart_myid(&myid);
    dash::_myid = myid;
  }
  return dash::_myid;
}

size_t dash::size()
{
  size_t size;
  dart_size(&size);
  return size;
}


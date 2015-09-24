
#include <dash/Init.h>
#include <dash/Team.h>

namespace dash {
  static int  _myid        = -1;
  static bool _initialized = false;
}

void dash::init(int *argc, char ***argv)
{
  DASH_LOG_DEBUG("dash::init()");
  dart_init(argc,argv);
  dash::_initialized = true;
  DASH_LOG_DEBUG("dash::init >");
}

void dash::finalize()
{
  DASH_LOG_DEBUG("dash::finalize()");
  dash::_initialized = false;
  dash::barrier();
  // Deallocate global memory allocated by this team:
  dash::Team::All().free();
  dart_exit();
  DASH_LOG_DEBUG("dash::finalize >");
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


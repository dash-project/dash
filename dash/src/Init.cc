
#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/util/Locality.h>

namespace dash {
  static int  _myid        = -1;
  static int  _size        = -1;
  static bool _initialized = false;
}

void dash::init(int *argc, char ***argv)
{
  DASH_LOG_DEBUG("dash::init()");
  dart_init(argc,argv);
  dash::_initialized = true;
  dash::util::Locality::init();
  DASH_LOG_DEBUG("dash::init >");
}

void dash::finalize()
{
  DASH_LOG_DEBUG("dash::finalize()");
  // Check init status of DASH
  if (!dash::is_initialized()) {
    // DASH has not been initalized or multiple calls of finalize, ignore:
    DASH_LOG_DEBUG("dash::finalize", "not initialized, ignore");
    return;
  }

  // Wait for all units:
  dash::barrier();

  // Deallocate global memory allocated in teams:
  DASH_LOG_DEBUG("dash::finalize", "free team global memory");
  dash::Team::finalize();

  // Wait for all units:
  dash::barrier();

  // Finalize DASH runtime:
  DASH_LOG_DEBUG("dash::finalize", "finalize DASH runtime");
  dart_exit();

  // Mark DASH as finalized (allow subsequent dash::init):
  dash::_initialized = false;

  DASH_LOG_DEBUG("dash::finalize >");
}

bool dash::is_initialized()
{
  return dash::_initialized;
}

void dash::barrier()
{
  dash::Team::All().barrier();
}

int dash::myid()
{
  if (dash::_myid < 0 && dash::is_initialized()) {
    // First call of dash::myid() after dash::init():
    dart_unit_t myid;
    dart_myid(&myid);
    dash::_myid = myid;
  } else if (!dash::is_initialized()) {
    // First call of dash::myid() after dash::finalize():
    dash::_myid = -1;
  }
  return dash::_myid;
}

size_t dash::size()
{
  if (dash::_size < 0 && dash::is_initialized()) {
    // First call of dash::size() after dash::init():
    size_t size;
    dart_size(&size);
    dash::_size = size;
  } else if (!dash::is_initialized()) {
    // First call of dash::size() after dash::finalize():
    dash::_size = -1;
  }
  return dash::_size;
}


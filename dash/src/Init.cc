
#include <dash/Init.h>
#include <dash/Team.h>
#include <dash/Types.h>
#include <dash/Shared.h>

#include <dash/util/Locality.h>
#include <dash/util/Config.h>
#include <dash/internal/Logging.h>

#include <dash/internal/Annotation.h>


namespace dash {
  static bool _initialized   = false;
  static bool _multithreaded = false;
}

namespace dash {
namespace internal {

void wait_breakpoint()
{
  sleep(1);
}

} // namespace internal
} // namespace dash

void dash::init(int * argc, char ** *argv)
{
  DASH_LOG_DEBUG("dash::init()");

  DASH_LOG_DEBUG("dash::init", "dash::util::Config::init()");
  dash::util::Config::init();

#if defined(DASH_ENABLE_THREADSUPPORT)
  DASH_LOG_DEBUG("dash::init", "dart_init_thread()");
  dart_thread_support_level_t provided_mt;
  dart_init_thread(argc, argv, &provided_mt);
  dash::_multithreaded = (provided_mt == DART_THREAD_MULTIPLE);
  if (!dash::_multithreaded) {
    DASH_LOG_WARN("dash::init",
                  "Support for multi-threading requested at compile time but "
                  "DART does not support multi-threaded access.");
  }
#else
  DASH_LOG_DEBUG("dash::init", "dart_init()");
  dart_init(argc, argv);
#endif

  dash::_initialized = true;

  // initialize global team
  dash::Team::initialize();

  if (dash::util::Config::get<bool>("DASH_INIT_BREAKPOINT")) {
    DASH_LOG_DEBUG("Process ID", getpid());
    if (dash::myid() == 0) {
      int blockvar = 1;
      dash::prevent_opt_elimination(blockvar);
      while (blockvar) {
        dash::internal::wait_breakpoint();
      }
    }
    dash::barrier();
  }

  DASH_LOG_DEBUG("dash::init", "dash::util::Locality::init()");
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

bool dash::is_multithreaded()
{
  return dash::_multithreaded;
}

void dash::barrier()
{
  dash::Team::All().barrier();
}

dash::global_unit_t dash::myid()
{
  return dash::Team::GlobalUnitID();
}

size_t dash::size()
{
  return dash::Team::All().size();
}


#include <dash/Mutex.h>
#include <dash/Exception.h>

namespace dash {

Mutex::Mutex(Team& team)
  : _team(&team)
{
  init();
}

bool Mutex::init() {
  if (dart_lock_initialized(_mutex.get())) {
    DASH_LOG_ERROR(
        "dash::Mutex::init()",
        "DART lock is already initialized");
    return false;
  }
  if (*_team != dash::Team::Null() && dash::is_initialized()) {
    dart_lock_t m;
    dart_ret_t ret = dart_team_lock_init(_team->dart_id(), &m);

    if (ret != DART_OK) {
        DASH_LOG_ERROR(
            "dash::Mutex::init()",
            "Failed to initialize DART lock! "
            "(dart_team_lock_init failed)");
        return false;
    }

    _mutex.reset(m);
    return true;
  }

  return false;
}

void Mutex::lock(){
  DASH_ASSERT(dart_lock_initialized(_mutex.get()));
  dart_ret_t ret = dart_lock_acquire(_mutex.get());
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_acquire failed");
}

bool Mutex::try_lock(){
  int32_t result;

  DASH_ASSERT(dart_lock_initialized(_mutex.get()));
  dart_ret_t ret = dart_lock_try_acquire(_mutex.get(), &result);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_try_acquire failed");
  return static_cast<bool>(result);
}

void Mutex::unlock(){
  DASH_ASSERT(dart_lock_initialized(_mutex.get()));
  dart_ret_t ret = dart_lock_release(_mutex.get());
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_acquire failed");
}

} // namespace dash

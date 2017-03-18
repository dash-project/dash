#include <dash/Mutex.h>
#include <dash/Exception.h>

namespace dash {

Mutex::Mutex(Team & team)
: _team(team) {
  dart_ret_t ret = dart_team_lock_init(_team.dart_id(), &_mutex);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_team_lock_init failed");
}

Mutex::~Mutex(){
  dart_ret_t ret = dart_team_lock_free(_team.dart_id(), &_mutex);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_team_lock_free failed");
}

void Mutex::lock(){
  dart_ret_t ret = dart_lock_acquire(_mutex);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_acquire failed");
}

bool Mutex::try_lock(){
  int32_t result;
  dart_ret_t ret = dart_lock_try_acquire(_mutex, &result);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_try_acquire failed");
  return static_cast<bool>(result);
}

void Mutex::unlock(){
  dart_ret_t ret = dart_lock_release(_mutex);
  DASH_ASSERT_EQ(DART_OK, ret, "dart_lock_acquire failed");
}

} // namespace dash

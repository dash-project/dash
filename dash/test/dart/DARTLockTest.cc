#include "DARTLockTest.h"

#include <dash/Shared.h>
#include <dash/dart/if/dart.h>

TEST_F(DARTLockTest, LockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 10;
  dash::Shared<value_t> shared;
  dart_lock_t lock;

  if (dash::myid() == 0) {
    shared.set(0);
  }

  dart_team_lock_init(DART_TEAM_ALL, &lock);

  for (int i = 0; i < num_iterations; ++i) {
    dart_lock_acquire(lock);
    shared.set(shared.get() + 1);
    dart_lock_release(lock);
  }

  dash::barrier();

  ASSERT_EQ_U(num_iterations * dash::size(), static_cast<value_t>(shared.get()));

  dart_team_lock_free(DART_TEAM_ALL, &lock);

}

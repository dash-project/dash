#include "DARTLockTest.h"

#include <dash/Shared.h>
#include <dash/dart/if/dart.h>


TEST_F(DARTLockTest, LockUnlockDoNothing) {
  using value_t = int;
  constexpr int num_iterations = 100;
  dart_lock_t lock;

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(DART_TEAM_ALL, &lock));

  dash::barrier();
  for (int i = 0; i < num_iterations; ++i) {
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_acquire(lock));
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_release(lock));
  }
  dash::barrier();

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

}

TEST_F(DARTLockTest, LockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 10;
  dash::Shared<value_t> shared;
  dart_lock_t lock;

  if (dash::myid() == 0) {
    shared.set(0);
  }

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(DART_TEAM_ALL, &lock));

  dash::barrier();
  for (int i = 0; i < num_iterations; ++i) {
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_acquire(lock));
    shared.set(shared.get() + 1);
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_release(lock));
  }
  dash::barrier();

  ASSERT_EQ_U(num_iterations * dash::size(), static_cast<value_t>(shared.get()));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

}

TEST_F(DARTLockTest, TryLockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 10;
  dash::Shared<value_t> shared;
  dart_lock_t lock;

  if (dash::myid() == 0) {
    shared.set(0);
  }

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(DART_TEAM_ALL, &lock));

  dash::barrier();
  for (int i = 0; i < num_iterations; ++i) {
    int32_t acquired;
    do {
      ASSERT_EQ_U(
        DART_OK,
        dart_lock_try_acquire(lock, &acquired));
    } while (!acquired);
    shared.set(shared.get() + 1);
    dart_lock_release(lock);
  }
  dash::barrier();

  ASSERT_EQ_U(num_iterations * dash::size(), static_cast<value_t>(shared.get()));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

}


TEST_F(DARTLockTest, ThreadedLockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 20;


  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }


#if !defined (DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else

  dash::Shared<value_t> shared;
  dart_lock_t lock;

  if (dash::myid() == 0) {
    shared.set(0);
  }

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(DART_TEAM_ALL, &lock));

  dash::barrier();
#pragma omp parallel for default(shared)
    for (int i = 0; i < num_iterations; ++i) {
      ASSERT_EQ_U(
        DART_OK,
        dart_lock_acquire(lock));
      shared.set(shared.get() + 1);
      ASSERT_EQ_U(
        DART_OK,
        dart_lock_release(lock));
    }
  dash::barrier();

  ASSERT_EQ_U(num_iterations * dash::size(),
    static_cast<value_t>(shared.get()));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

#endif // !defined (DASH_ENABLE_OPENMP)

}

TEST_F(DARTLockTest, ThreadedTryLockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 20;

  if (!dash::is_multithreaded()) {
    SKIP_TEST_MSG("requires support for multi-threading");
  }

#if !defined (DASH_ENABLE_OPENMP)
  SKIP_TEST_MSG("requires support for OpenMP");
#else
  dash::Shared<value_t> shared;
  dart_lock_t lock;

  if (dash::myid() == 0) {
    shared.set(0);
  }

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(DART_TEAM_ALL, &lock));

  dash::barrier();
#pragma omp parallel for default(shared)
  for (int i = 0; i < num_iterations; ++i) {
    int32_t acquired;
    do {
      ASSERT_EQ_U(
        DART_OK,
        dart_lock_try_acquire(lock, &acquired));
    } while (!acquired);
    shared.set(shared.get() + 1);
    dart_lock_release(lock);
  }
  dash::barrier();

  ASSERT_EQ_U(num_iterations * dash::size(), static_cast<value_t>(shared.get()));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

#endif // !defined (DASH_ENABLE_OPENMP)
}

TEST_F(DARTLockTest, TeamLockUnlock) {
  using value_t = int;
  constexpr int num_iterations = 10;

  if (dash::size() < 4) {
    SKIP_TEST_MSG("requires at least 4 units");
  }

  auto& team = dash::Team::All().split(2);

  dash::Shared<value_t> shared(dash::team_unit_t(0), team);
  dart_lock_t lock;

  if (team.myid() == 0) {
    shared.set(0);
  }

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_init(team.dart_id(), &lock));

  dash::barrier();
  for (int i = 0; i < num_iterations; ++i) {
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_acquire(lock));
    shared.set(shared.get() + 1);
    ASSERT_EQ_U(
      DART_OK,
      dart_lock_release(lock));
  }
  team.barrier();

  ASSERT_EQ_U(num_iterations * team.size(), static_cast<value_t>(shared.get()));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_lock_destroy(&lock));

}

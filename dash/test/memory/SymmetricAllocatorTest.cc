
#include "SymmetricAllocatorTest.h"

#include <dash/GlobPtr.h>
#include <dash/Pattern.h>
#include <dash/allocator/SymmetricAllocator.h>
#include <dash/memory/HostSpace.h>

TEST_F(SymmetricAllocatorTest, Constructor)
{
  using allocator_type = dash::SymmetricAllocator<
      int,
      dash::global_allocation_policy::collective,
      dash::HostSpace>;

  allocator_type target{dash::Team::All()};
  dart_gptr_t    requested = target.allocate(sizeof(int) * 10);

  ASSERT_EQ(0, requested.unitid);
  ASSERT_EQ(DART_TEAM_ALL, requested.teamid);
}

TEST_F(SymmetricAllocatorTest, TeamAlloc)
{
  if (_dash_size < 2) {
    SKIP_TEST_MSG("Test case requires at least two units");
  }
  dash::Team& subteam = dash::Team::All().split(2);

  dash::SymmetricAllocator<
      int,
      dash::global_allocation_policy::collective,
      dash::HostSpace>
              target(subteam);
  dart_gptr_t requested = target.allocate(10);

  // make sure the unitid in the gptr is
  // team-local and 0 instead of the corresponding global unit ID
  ASSERT_EQ(0, requested.unitid);
  ASSERT_EQ(subteam.dart_id(), requested.teamid);
}

TEST_F(SymmetricAllocatorTest, MoveAssignment)
{
  using glob_mem_t = dash::GlobStaticMem<
      int,
      dash::HostSpace,
      dash::global_allocation_policy::collective>;
  using GlobPtr_t = dash::GlobConstPtr<int, glob_mem_t>;
  using Alloc_t   = dash::SymmetricAllocator<
      int,
      dash::global_allocation_policy::collective,
      dash::HostSpace>;
  GlobPtr_t gptr;
  Alloc_t   target_new(dash::Team::All());

  {
    auto        target_old = Alloc_t(dash::Team::All());
    dart_gptr_t requested  = target_old.allocate(sizeof(int) * 10);
    gptr                   = GlobPtr_t(requested);

    if (dash::myid().id == 0) {
      // assign value
      int value = 10;
      (*gptr)   = value;
    }
    dash::barrier();

    target_new = Alloc_t(dash::Team::All());
    target_new = std::move(target_old);
  }
  // target_old has left scope

  int value = (*gptr);
  ASSERT_EQ_U(static_cast<int>(*gptr), value);

  dash::barrier();

  target_new.deallocate(gptr.dart_gptr());
}

TEST_F(SymmetricAllocatorTest, MoveCtor)
{
  using glob_mem_t = dash::GlobStaticMem<
      int,
      dash::HostSpace,
      dash::global_allocation_policy::collective>;

  using GlobPtr_t = dash::GlobConstPtr<int, glob_mem_t>;

  using Alloc_t = dash::SymmetricAllocator<
      int,
      dash::global_allocation_policy::collective,
      dash::HostSpace>;

  GlobPtr_t gptr;
  Alloc_t   target_new(dash::Team::All());

  {
    auto        target_old = Alloc_t((dash::Team::All()));
    dart_gptr_t requested  = target_old.allocate(sizeof(int) * 5);
    gptr                   = GlobPtr_t(requested);

    if (dash::myid().id == 0) {
      // assign value
      int value = 10;
      (*gptr)   = value;
    }
    dash::barrier();

    target_new = Alloc_t(std::move(target_old));
  }
  // target_old has left scope

  int value = (*gptr);
  ASSERT_EQ_U(static_cast<int>(*gptr), value);

  dash::barrier();

  target_new.deallocate(gptr.dart_gptr());
}

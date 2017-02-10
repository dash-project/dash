
#include "LocalAllocatorTest.h"

#include <dash/allocator/LocalAllocator.h>
#include <dash/GlobPtr.h>
#include <dash/GlobRef.h>
#include <dash/Pattern.h>

TEST_F(LocalAllocatorTest, Constructor)
{
  auto target           = dash::allocator::LocalAllocator<int>();
  if(dash::myid().id == 0){
    // must not hang, as no synchronisation is allowed
    dart_gptr_t requested = target.allocate(sizeof(int) * 10);
    ASSERT_EQ(0, requested.unitid);
  }
}

TEST_F(LocalAllocatorTest, MoveAssignment)
{
  using GlobPtr_t = dash::GlobPtr<int, dash::Pattern<1>>;
  using Alloc_t   = dash::allocator::LocalAllocator<int>;
  GlobPtr_t gptr;
  Alloc_t target_new;

  {
    auto target_old       = Alloc_t(dash::Team::All());
    dart_gptr_t requested = target_old.allocate(sizeof(int) * 10);
    gptr = GlobPtr_t(requested);

    if(dash::myid().id == 0){
      // assign value
      int value = 10;
      (*gptr) = value;
    }
    dash::barrier();

    target_new       = Alloc_t(); 
    target_new       = std::move(target_old);
  }
  // target_old has left scope

  int value = (*gptr);
  ASSERT_EQ_U(static_cast<int>(*gptr), value);

  dash::barrier();

  target_new.deallocate(gptr.dart_gptr());
}

TEST_F(LocalAllocatorTest, MoveCtor)
{
  using GlobPtr_t = dash::GlobPtr<int, dash::Pattern<1>>;
  using Alloc_t   = dash::allocator::LocalAllocator<int>;
  GlobPtr_t gptr;
  Alloc_t target_new;

  {
    auto target_old       = Alloc_t(dash::Team::All());
    dart_gptr_t requested = target_old.allocate(sizeof(int) * 5);
    gptr = GlobPtr_t(requested);

    if(dash::myid().id == 0){
      // assign value
      int value = 10;
      (*gptr) = value;
    }
    dash::barrier();

    target_new       = Alloc_t(std::move(target_old)); 
  }
  // target_old has left scope

  int value = (*gptr);
  ASSERT_EQ_U(static_cast<int>(*gptr), value);

  dash::barrier();

  target_new.deallocate(gptr.dart_gptr());
}

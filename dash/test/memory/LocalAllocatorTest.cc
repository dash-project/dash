
#include "LocalAllocatorTest.h"

#include <dash/allocator/LocalAllocator.h>
#include <dash/GlobPtr.h>
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

TEST_F(LocalAllocatorTest, MemAlloc)
{
  using elem_t = int;

  int elem_per_thread = 1 + dash::myid().id;

  auto ptr1 = dash::memalloc<elem_t>(elem_per_thread);
  DASH_LOG_DEBUG_VAR("LocalAllocatorTest.MemAlloc", ptr1);

  auto ptr2 = dash::memalloc<elem_t>(elem_per_thread);
  DASH_LOG_DEBUG_VAR("LocalAllocatorTest.MemAlloc", ptr2);

  EXPECT_NE_U(ptr1, ptr2);

  dash::memfree(ptr1);
  dash::memfree(ptr2);
}

TEST_F(LocalAllocatorTest, MoveAssignment)
{
  using GlobPtr_t = dash::GlobConstPtr<int>;
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
  using GlobPtr_t = dash::GlobConstPtr<int>;
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

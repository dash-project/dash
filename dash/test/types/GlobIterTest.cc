
#include "../TestBase.h"
#include "../TestLogHelpers.h"
#include "GlobIterTest.h"

#include <type_traits>
#include <dash/Types.h>
#include <dash/Array.h>
#include <dash/Atomic.h>


TEST_F(GlobIterTest, IteratorTypes)
{
  {
    using array_t = dash::Array<int>;
    static_assert(dash::is_container_compatible<array_t::iterator>::value,
        "Array::iterator not trivially copyable");
    static_assert(dash::is_container_compatible<array_t::const_iterator>::value,
        "Array::const_iterator not trivially copyable");
  }
  {
    using array_t = dash::Array<dash::Atomic<int>>;
    static_assert(dash::is_container_compatible<array_t::iterator>::value,
        "Array<Atomic<T>>::iterator not trivially copyable");
    static_assert(dash::is_container_compatible<array_t::const_iterator>::value,
        "Array<Atomic<T>>::const_iterator not trivially copyable");
  }
}

TEST_F(GlobIterTest, RemoteIterator){
  using array_t = dash::Array<int>;
  array_t                        values(dash::size());
  dash::Array<array_t::iterator> iterators(dash::size());

  int myid = dash::myid();
  *(values.lbegin())    = myid;
  *(iterators.lbegin()) = values.begin()+myid;

  values.barrier();
  iterators.barrier();

  int right_neighbor = (myid + 1) % dash::size();
  array_t::iterator it = iterators[right_neighbor];
  // check positions of iterators
  // that has to work independently of the memory
  ASSERT_EQ_U(it.pos(), right_neighbor);

  // increment neighbors value
  // that does not work, as the position depends on the global memory instance
  // which is not available on the remote unit (at given pointer)
  /*
  auto gref = *it;
  ++gref;

  values.barrier();
  int newval =  *(values.lbegin());
  ASSERT_EQ_U(newval, myid+1);
  */
}

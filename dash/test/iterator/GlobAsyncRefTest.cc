
#include "GlobAsyncRefTest.h"

#include <dash/GlobAsyncRef.h>
#include <dash/Array.h>
#include <type_traits>


TEST_F(GlobAsyncRefTest, IsLocal) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(dash::size() * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();
  // Test global async references on array elements:
  auto global_offset      = array.pattern().global(0);
  // Reference first local element in global memory:
  dash::GlobRef<int> gref = array[global_offset];
  dash::GlobAsyncRef<int> gar_local_g(gref);
  ASSERT_EQ_U(true, gar_local_g.is_local());
}

/**
 * Non-blocking writes to distributed array with push semantics.
 */
TEST_F(GlobAsyncRefTest, Push) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(dash::size() * num_elem_per_unit);
  array.barrier();
  size_t lneighbor = (dash::myid() + dash::size() - 1) % dash::size();
  size_t rneighbor = (dash::myid() + 1) % dash::size();
  // Assign values at left neighbor asynchronously:
  size_t start_idx = lneighbor * num_elem_per_unit;
  for (auto gi = start_idx; gi < (start_idx + num_elem_per_unit); ++gi) {
    // Changes local value only
    array.async[gi] = dash::myid().id;
  }
  // Flush local window:
  array.async.flush();
  dash::barrier();
  // Test values in local window. Changes by all units should be visible:
  for (auto li = 0; li < array.lcapacity(); ++li) {
    // All local values incremented once by all units
    ASSERT_EQ_U(rneighbor,
                array.local[li]);
  }
}


TEST_F(GlobAsyncRefTest, GetSet) {
  // Initialize values:
  dash::Array<int> array(dash::size());
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();

  int neighbor = (dash::myid() + 1) % dash::size();

  // Reference a neighbors element in global memory:
  dash::GlobAsyncRef<int> garef = array.async[neighbor];

  int val = garef.get();
  garef.flush();
  ASSERT_EQ_U(neighbor, val);

  val = 0;

  garef.get(val);
  garef.flush();
  ASSERT_EQ_U(neighbor, val);

  array.barrier();
  garef.set(dash::myid());
  garef.flush();
  ASSERT_EQ_U(garef.get(), dash::myid().id);
  array.barrier();
  garef.set(dash::myid());
  garef.flush();
  ASSERT_EQ_U(garef.get(), dash::myid().id);
  array.barrier();
  int left_neighbor = (dash::myid() + dash::size() - 1) % dash::size();
  ASSERT_EQ_U(left_neighbor, array.local[0]);
}

TEST_F(GlobAsyncRefTest, Conversion)
{
  // Initialize values:
  dash::Array<int> array(dash::size());
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();

  auto gref_async = static_cast<dash::GlobAsyncRef<int>>(
                        array[dash::myid().id]);
  auto gref_sync  = static_cast<dash::GlobRef<int>>(
                        array.async[dash::myid().id]);
  ASSERT_EQ_U(gref_async.is_local(), true);
  ASSERT_EQ_U(gref_sync.is_local(), true);
}

struct mytype {int a; double b; };

std::ostream&
operator<<(std::ostream& os, mytype const s) {
  os << "{" << "a: " << s.a << ", b:" << s.b << "}";
  return os;
}

TEST_F(GlobAsyncRefTest, RefOfStruct)
{
  if(dash::size() < 2){
    SKIP_TEST_MSG("this test requires at least 2 units");
  }

  dash::Array<mytype> array(dash::size());

  int neighbor = (dash::myid() + 1) % dash::size();
  // Reference a neighbors element in global memory:
  auto garef_rem = array.async[neighbor];
  auto garef_loc = array.async[dash::myid().id];

  {
    auto garef_a_rem = garef_rem.member<int>(&mytype::a);
    auto garef_b_rem = garef_rem.member<double>(&mytype::b);

    auto garef_a_loc = garef_loc.member<int>(&mytype::a);
    auto garef_b_loc = garef_loc.member<double>(&mytype::b);

    ASSERT_EQ_U(garef_rem.is_local(), false);
    ASSERT_EQ_U(garef_a_rem.is_local(), false);
    ASSERT_EQ_U(garef_b_rem.is_local(), false);

    ASSERT_EQ_U(garef_loc.is_local(), true);
    ASSERT_EQ_U(garef_a_loc.is_local(), true);
    ASSERT_EQ_U(garef_b_loc.is_local(), true);
  }
  array.barrier();
  {
    mytype data {1, 2.0};
    garef_rem = data;
    garef_rem.flush();
    auto garef_a_rem = garef_rem.member<int>(&mytype::a);
    auto garef_b_rem = garef_rem.member<double>(&mytype::b);

    // GlobRefAsync is constructed after data is set, so it stores value
    int a = garef_a_rem.get();
    int b = garef_b_rem.get();
    ASSERT_EQ_U(a, 1);
    ASSERT_EQ_U(b, 2);
  }

}

TEST_F(GlobAsyncRefTest, ConstTest)
{

  dash::Array<int> array(dash::size());
  const dash::Array<int>& carr = array;
  array[dash::myid()].set(0);
  dash::barrier();

  // conversion non-const -> const
  dash::GlobRef<const int> gref1 = array[0];
  // assignment const -> const
  dash::GlobRef<const int> gref2 = carr[0];
  // explicit conversion const->non-const
  dash::GlobRef<int> gref3(carr[0]);

  // should fail!
  //gref1.set(0);

  // works
  ASSERT_EQ_U(0, gref1.get());

  /**
   * GlobAsyncRef
   */

  // conversion non-const -> const
  dash::GlobAsyncRef<const int> agref1 = array.async[0];
  // assignment const -> const
  dash::GlobAsyncRef<const int> agref2 = carr.async[0];
  // explicit conversion const->non-const
  dash::GlobAsyncRef<int> agref3 =
                            static_cast<dash::GlobAsyncRef<int>>(carr.async[0]);

  dash::GlobAsyncRef<const int> agref4 = gref1;
  dash::GlobAsyncRef<const int> agref5{gref1};
  // should fail!
  //agref1.set(0);

  // works
  ASSERT_EQ_U(0, agref1.get());

}

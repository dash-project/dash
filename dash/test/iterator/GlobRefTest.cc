
#include "GlobRefTest.h"

#include <dash/GlobRef.h>
#include <dash/Array.h>
#include <dash/Matrix.h>


TEST_F(GlobRefTest, IsLocal) {
  int num_elem_per_unit = 20;
  // Initialize values:
  dash::Array<int> array(dash::size() * num_elem_per_unit);
  for (auto li = 0; li < array.lcapacity(); ++li) {
    array.local[li] = dash::myid().id;
  }
  array.barrier();
  // Test global references on array elements:
  auto global_offset      = array.pattern().global(0);
  // Reference first local element in global memory:
  dash::GlobRef<int> gref = array[global_offset];
  ASSERT_EQ_U(true, gref.is_local());
}

TEST_F(GlobRefTest, Member) {
  struct value_t {
    double x; int y;
  };
  // Initialize values:
  dash::Array<value_t> array(dash::size());
//  array.local[0].x = 1 + dash::myid().id/10.0;
//  array.local[0].y = 1000*dash::myid().id;
  array[dash::myid()].member(&value_t::x) = 1 + dash::myid().id/10.0;
  array[dash::myid()].member(&value_t::y) = 1000*dash::myid().id;
  array.barrier();
  // Test global references on array elements:
  auto neighbor      = (dash::myid() + 1) % dash::size();
  auto val_gref      = array[neighbor];

  ASSERT_EQ_U(1000*neighbor, val_gref.member<int>(sizeof(double)));

  ASSERT_EQ_U(1 + neighbor/10.0, val_gref.member(&value_t::x));
  ASSERT_EQ_U(1000*neighbor, val_gref.member(&value_t::y));

  value_t val = array[neighbor];
  ASSERT_EQ_U(1 + neighbor/10.0, val.x);
  ASSERT_EQ_U(1000*neighbor, val.y);
}

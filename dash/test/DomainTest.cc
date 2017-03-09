
#include "DomainTest.h"

#include <dash/Domain.h>

#include <array>


TEST_F(DomainTest, Basic3Dim)
{
  if (_dash_id != 0) { return; }

  dash::Domain<3, int> dom({ {0,10}, {10,20}, {5,10} });

  dom.translate({ 0, 3, 0 })
     .expand({ 0, 0, 10 });

  std::array<int, 3> extents_exp = { 10,10,15 };
  std::array<int, 3> offsets_exp = {  0,13, 5 };

  EXPECT_EQ_U(dom.extents(), extents_exp);
  EXPECT_EQ_U(dom.offsets(), offsets_exp);
}

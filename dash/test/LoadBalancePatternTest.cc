#include <libdash.h>
#include <gtest/gtest.h>

#include "LoadBalancePatternTest.h"
#include "TestBase.h"

#include <dash/LoadBalancePattern.h>


TEST_F(LoadBalancePatternTest, InitLocalSizes)
{
  typedef dash::LoadBalancePattern<1> pattern_t;

  size_t num_elem_total = dash::size() * 23;

  dash::Team & team = dash::Team::All();

  dash::util::TeamLocality tloc(team);

  pattern_t pat(dash::SizeSpec<1>(num_elem_total),
                tloc);
}


#include "GlobStaticMemTest.h"

#include <dash/memory/GlobStaticMem.h>


TEST_F(GlobStaticMemTest, ConstructorInitializerList)
{
  auto target_local_elements = { 1, 2, 3, 4, 5, 6 };
  auto target = dash::GlobStaticMem<int>(target_local_elements);

  std::vector<int> glob_values;
  for (dash::team_unit_t u{0}; u < dash::size(); u++) {
    for (int l = 0; l < target_local_elements.size(); l++) {
      int val = *(target.at(u,l));
      EXPECT_EQ_U(l+1, val);
      glob_values.push_back(val);
    }
  }
  for (auto val : glob_values) {
    DASH_LOG_DEBUG_VAR("GlobStaticMemTest.ConstructorInitializerList", val);
  }

  int target_element;
  for (int l = 0; l < target_local_elements.size(); l++) {
    target.get_value(&target_element, l);
    EXPECT_EQ_U(l+1, target_element);
  }
}

TEST_F(GlobStaticMemTest, GlobalRandomAccess)
{
  auto globmem_local_elements = { 1, 2, 3 };
  auto globmem = dash::GlobStaticMem<int>(globmem_local_elements);

  DASH_LOG_DEBUG_VAR("GlobStaticMemTest", globmem.size());
  EXPECT_EQ_U(globmem.size(), 3 * dash::size());

  if (dash::myid() == 0) {
    auto gbegin = globmem.begin();
    auto glast  = gbegin + (globmem.size() - 1);
    auto gend   = gbegin + globmem.size();

    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gbegin);
    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", glast);
    DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gend);

    // Test distance in global memory over multiple unit spaces:
    EXPECT_EQ(gend  - gbegin, globmem.size());
    EXPECT_EQ(glast - gbegin, globmem.size() - 1);

    // Iterate entire global memory space, including end pointer:
    for (int g = 0; g <= globmem.size(); ++g) {
      DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gbegin);
      // Do not dereference end pointer:
      if (g < globmem.size()) {
        int gvalue = *gbegin;
        EXPECT_EQ((g % 3) + 1, gvalue);
        EXPECT_EQ(*gbegin, globmem.begin()[g]);
      }
      EXPECT_EQ( gbegin, globmem.begin() + g);

      EXPECT_EQ( (globmem.size() - g), dash::distance(gbegin, gend));
      EXPECT_EQ(-(globmem.size() - g), dash::distance(gend,   gbegin));
      EXPECT_EQ(gend   - gbegin,       dash::distance(gbegin, gend));
      EXPECT_EQ(gbegin - gend,         dash::distance(gend,   gbegin));

      if (g % 2 == 0) { ++gbegin; }
      else            { gbegin++; }
    }
  }

  dash::barrier();

  if (dash::myid() == dash::size() - 1) {
    auto gbegin = globmem.begin();
    auto gend   = gbegin + globmem.size();
    // Reverse iteratation on entire global memory space, starting at
    // end pointer:
    for (int g = globmem.size(); g >= 0; --g) {
      DASH_LOG_DEBUG_VAR("GlobStaticMemTest", gend);
      // Do not dereference end pointer:
      if (g < globmem.size()) {
        int gvalue = *gend;
        EXPECT_EQ((g % 3) + 1, gvalue);
      }
      EXPECT_EQ(gend, globmem.begin() + g);

      EXPECT_EQ(gend   - gbegin, dash::distance(gbegin, gend));
      EXPECT_EQ(gbegin - gend,   dash::distance(gend,   gbegin));

      if (g % 2 == 0) { --gend; }
      else            { gend--; }
    }
  }
}

TEST_F(GlobStaticMemTest, LocalBegin)
{
  auto   target_local_elements = { 1, 2, 3, 4 };

  if(!dash::Team::All().is_leaf()){
    SKIP_TEST_MSG("Team is already split");
  }

  auto & sub_team = dash::size() < 4
                    ? dash::Team::All()
                    : dash::Team::All().split(2);

  auto   target = dash::GlobStaticMem<int>(target_local_elements, sub_team);

  for (int l = 0; l < target_local_elements.size(); l++) {
    EXPECT_EQ_U(*(target_local_elements.begin() + l), target.lbegin()[l]);
  }
  EXPECT_NE_U(target.lbegin(), nullptr);
}

TEST_F(GlobStaticMemTest, MoveSemantics){
  using memory_t = dash::GlobStaticMem<int>;
  // move construction
  {
    memory_t memory_a(10);

    *(memory_a.lbegin()) = 5;
    dash::barrier();

    memory_t memory_b(std::move(memory_a));
    int value = *(memory_b.lbegin());
    ASSERT_EQ_U(value, 5);
  }
  dash::barrier();
  //move assignment
  {
    memory_t memory_a(10);
    {
      memory_t memory_b(8);

      *(memory_a.lbegin()) = 1;
      *(memory_b.lbegin()) = 2;
      memory_a = std::move(memory_b);
      // leave scope of memory_b
    }
    ASSERT_EQ_U(*(memory_a.lbegin()), 2);
  }
  dash::barrier();
  // swap
  {
    memory_t memory_a(10);
    memory_t memory_b(8);

    *(memory_a.lbegin()) = 1;
    *(memory_b.lbegin()) = 2;

    std::swap(memory_a, memory_b);
    ASSERT_EQ_U(*(memory_a.lbegin()), 2);
    ASSERT_EQ_U(*(memory_b.lbegin()), 1);
  }
}

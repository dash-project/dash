#include "GlobMemTest.h"

TEST_F(GlobMemTest, ConstructorInitializerList)
{
  auto target_local_elements = { 1, 2, 3, 4, 5, 6 };
  auto target = dash::GlobMem<int>(target_local_elements);

  std::vector<int> glob_values;
  for (dash::team_unit_t u{0}; u < dash::size(); u++) {
    for (int l = 0; l < target_local_elements.size(); l++) {
      int val = *(target.at(u,l));
      EXPECT_EQ_U(l+1, val);
      glob_values.push_back(val);
    }
  }
  for (auto val : glob_values) {
    DASH_LOG_DEBUG_VAR("GlobMemTest.ConstructorInitializerList", val);
  }

  int target_element;
  for (int l = 0; l < target_local_elements.size(); l++) {
    target.get_value(&target_element, l);
    EXPECT_EQ_U(l+1, target_element);
  }
}

TEST_F(GlobMemTest, LocalBegin)
{
  auto   target_local_elements = { 1, 2, 3, 4 };

  if(!dash::Team::All().is_leaf()){
    SKIP_TEST_MSG("Team is already split");
  }

  auto & sub_team = dash::size() < 4
                    ? dash::Team::All()
                    : dash::Team::All().split(2);

  auto   target = dash::GlobMem<int>(target_local_elements, sub_team);

  for (int l = 0; l < target_local_elements.size(); l++) {
    EXPECT_EQ_U(*(target_local_elements.begin() + l), target.lbegin()[l]);
  }
  EXPECT_NE_U(target.lbegin(), nullptr);
}

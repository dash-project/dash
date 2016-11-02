#include "GlobMemTest.h"

TEST_F(GlobMemTest, ConstructorInitializer_list)
{
  auto target_local_elements = {1,2,3,4,5,6};
  auto target = dash::GlobMem<int>(target_local_elements);
  
  int target_element;
  target.get_value(&target_element, 0);
  ASSERT_EQ_U(7, target_element);

  target.get_value(&target_element, 1);
  ASSERT_EQ_U(2, target_element);

  target.get_value(&target_element, 2);
  ASSERT_EQ_U(3, target_element);

  target.get_value(&target_element, 3);
  ASSERT_EQ_U(4, target_element);

  target.get_value(&target_element, 4);
  ASSERT_EQ_U(5, target_element);
}

TEST_F(GlobMemTest, lbegin)
{
  auto target_local_elements = {1,2,3,4};
  auto target = dash::GlobMem<int>(target_local_elements, dash::Team::All());

  auto element = target.lbegin(dash::myid());

  if(0 == dash::myid()) {
    ASSERT_EQ(*element, 1);
  }
  
  if(1 == dash::myid()) {
    ASSERT_EQ(*element, 2);
  }

  if(2 == dash::myid()) {
    ASSERT_EQ(*element, 3);
  }

  if(3 == dash::myid()) {
    ASSERT_EQ(*element, 4);
  }
}

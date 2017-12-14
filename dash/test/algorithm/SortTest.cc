#include "SortTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Sort.h>

#include <algorithm>
#include <random>

TEST_F(SortTest, ArraySort)
{
  typedef uint32_t               Element_t;
  typedef dash::Array<Element_t> Array_t;

  static std::uniform_int_distribution<Element_t> distribution(0, 100);
  static std::random_device                       rd;
  static std::mt19937 generator(rd() + dash::myid());

  size_t num_local_elem = 10;

  LOG_MESSAGE("SortTest.ArraySort: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  std::generate(
      array.lbegin(), array.lend(), []() { return distribution(generator); });

  // Wait for all units
  array.barrier();

  auto const true_sum = dash::accumulate(array.begin(), array.end(), 0);

  dash::sort(array.begin(), array.end());

  auto const actual_sum = dash::accumulate(array.begin(), array.end(), 0);

  if (dash::myid() == 0) {

    EXPECT_EQ_U(true_sum, actual_sum);

    for (auto it = array.begin() + 1; it < array.end(); ++it) {
      auto const a = static_cast<const Element_t>(*(it - 1));
      auto const b = static_cast<const Element_t>(*it);

      EXPECT_GE_U(b, a);
    }
  }
}

// TODO: add additional unit tests with various pattern types and containers

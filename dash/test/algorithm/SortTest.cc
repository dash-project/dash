#include "SortTest.h"

#include <dash/Array.h>
#include <dash/algorithm/Generate.h>
#include <dash/algorithm/Sort.h>

#include <algorithm>
#include <random>

template <typename GlobIter>
static void perform_test(GlobIter begin, GlobIter end);

template <
    class GlobIter,
    typename std::enable_if<std::is_integral<
        typename GlobIter::value_type>::value>::type* = nullptr>
static void rand_range(GlobIter begin, GlobIter end)
{
  static std::uniform_int_distribution<typename GlobIter::value_type>
                            distribution(-1000, 1000);
  static std::random_device rd;
  static std::mt19937       generator(rd() + begin.team().myid());

  dash::generate(begin, end, []() { return distribution(generator); });
}

template <
    class GlobIter,
    typename std::enable_if<std::is_floating_point<
        typename GlobIter::value_type>::value>::type* = nullptr>
static void rand_range(GlobIter begin, GlobIter end)
{
  static std::uniform_real_distribution<typename GlobIter::value_type>
                            distribution(-1.0, 1.0);
  static std::random_device rd;
  static std::mt19937       generator(rd() + begin.team().myid());

  dash::generate(begin, end, []() { return distribution(generator); });
}

TEST_F(SortTest, ArrayBlockedFullRange)
{
  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t num_local_elem = 10;

  LOG_MESSAGE("SortTest.ArrayBlockedFullRange: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  rand_range(array.begin(), array.end());

  array.barrier();

  perform_test(array.begin(), array.end());
}

TEST_F(SortTest, ArrayBlockedPartialRange)
{
  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t num_local_elem = 100;

  LOG_MESSAGE("SortTest.ArrayBlockedPartialRange: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  auto begin = array.begin() + array.lsize() / 2;
  auto end   = array.end() - array.lsize() / 2;

  rand_range(begin, end);

  // Wait for all units
  array.barrier();

  perform_test(begin, end);
}

TEST_F(SortTest, ArrayEmptyLocalRangeBegin)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units are required");
  }

  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t num_local_elem = 10;

  LOG_MESSAGE("SortTest.ArrayEmptyLocalBegin: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  auto begin = array.begin() + num_local_elem;
  auto end   = array.end();

  rand_range(begin, end);
  // Wait for all units
  array.barrier();

  perform_test(begin, end);
}

TEST_F(SortTest, ArrayEmptyLocalRangeEnd)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units are required");
  }

  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t num_local_elem = 10;

  LOG_MESSAGE("SortTest.ArrayEmptyLocalRangeEnd: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  auto begin = array.begin();
  auto end   = array.end() - num_local_elem;

  rand_range(begin, end);

  // Wait for all units
  array.barrier();

  perform_test(begin, end);
}

TEST_F(SortTest, ArrayOfDoubles)
{
  typedef double                 Element_t;
  typedef dash::Array<Element_t> Array_t;

  size_t num_local_elem = 100;

  LOG_MESSAGE("SortTest.ArrayOfDoubles: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  rand_range(array.begin(), array.end());

  // Wait for all units
  array.barrier();

  perform_test(array.begin(), array.end());
}

template <typename GlobIter>
static void perform_test(GlobIter begin, GlobIter end)
{
  using Element_t = typename decltype(begin)::value_type;

  auto const true_sum = dash::accumulate(begin, end, 0);

  dash::sort(begin, end);

  auto const actual_sum = dash::accumulate(begin, end, 0);

  if (dash::myid() == 0) {
    EXPECT_EQ_U(true_sum, actual_sum);

    for (auto it = begin + 1; it < end; ++it) {
      auto const a = static_cast<const Element_t>(*(it - 1));
      auto const b = static_cast<const Element_t>(*it);

      EXPECT_GE_U(b, a);
    }
  }
}

// TODO: add additional unit tests with various pattern types and containers

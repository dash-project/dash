#include "SortTest.h"

#include <dash/Array.h>
#include <dash/Matrix.h>
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

TEST_F(SortTest, ArrayUnderfilled)
{
  typedef int32_t                Element_t;
  typedef dash::Array<Element_t> Array_t;
  // Choose block size and number of blocks so at least
  // one unit has an empty local range and one unit has an
  // underfilled block.
  // Using a prime as block size for 'inconvenient' strides.
  int    block_size = 19;
  size_t num_units  = dash::Team::All().size();
  size_t num_elem   = ((num_units - 1) * block_size) - block_size / 2;
  if (num_units < 2) {
    num_elem = block_size - 1;
  }

  LOG_MESSAGE(
      "Units: %d, block size: %d, elements: %d", static_cast<int>(num_units),
      block_size, static_cast<int>(num_elem));

  // Initialize global array:
  Array_t array(num_elem, dash::BLOCKCYCLIC(block_size));

  LOG_MESSAGE("Number of local elements: %zu", array.lsize());

  rand_range(array.begin(), array.end());

  array.barrier();

  perform_test(array.begin(), array.end());
}
TEST_F(SortTest, ArrayEmptyLocalRangeMiddle)
{
  if (dash::size() < 2) {
    SKIP_TEST_MSG("At least 2 units are required");
  }

  using pattern_t = dash::CSRPattern<1>;
  using extent_t  = pattern_t::size_type;
  using index_t   = pattern_t::index_type;
  using value_t   = int32_t;

  auto const nunits = dash::size();

  std::vector<extent_t> local_sizes{};

  for (std::size_t u = 0; u < nunits; ++u) {
    local_sizes.push_back((u % 2) ? 0 : num_local_elem);
  }

  pattern_t                                pattern(local_sizes);
  dash::Array<value_t, index_t, pattern_t> array(pattern);

  rand_range(array.begin(), array.end());

  array.barrier();

  perform_test(array.begin(), array.end());
}

TEST_F(SortTest, ArrayOfDoubles)
{
  typedef double                 Element_t;
  typedef dash::Array<Element_t> Array_t;

  LOG_MESSAGE("SortTest.ArrayOfDoubles: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  rand_range(array.begin(), array.end());

  // Wait for all units
  array.barrier();

  perform_test(array.begin(), array.end());
}

TEST_F(SortTest, MatrixBlockedRow)
{
  using value_t     = int32_t;
  using block_pat_t = dash::BlockPattern<2, dash::ROW_MAJOR>;
  using narray_t =
      dash::NArray<value_t, 2, dash::default_index_t, block_pat_t>;

  size_t team_size = dash::Team::All().size();
  size_t extent_x  = 17;
  size_t extent_y  = 5 + team_size * 3;
  size_t size      = extent_x * extent_y;

  // Ceil division
  size_t block_size_x = extent_x;
  size_t block_size_y = dash::math::div_ceil(extent_y, team_size);

  LOG_MESSAGE(
      "ex: %d, ey: %d, bsx: %d, bsy: %d", static_cast<int>(extent_y),
      static_cast<int>(extent_x), static_cast<int>(block_size_y),
      static_cast<int>(block_size_x));

  block_pat_t pat_blocked_row{
      dash::SizeSpec<2>(extent_y, extent_x),
      dash::DistributionSpec<2>(dash::BLOCKED, dash::NONE),
      dash::TeamSpec<2>(dash::Team::All()), dash::Team::All()};

  narray_t mat{pat_blocked_row};

  rand_range(mat.begin(), mat.end());

  mat.barrier();

  perform_test(mat.begin(), mat.end());
}

TEST_F(SortTest, ArrayOfPoints)
{
  typedef Point                  Element_t;
  typedef dash::Array<Element_t> Array_t;

  LOG_MESSAGE("SortTest.ArrayOfPoints: allocate array");
  // Initialize global array:
  Array_t array(num_local_elem * dash::size());

  static std::uniform_int_distribution<int32_t> distribution(-1000, 1000);
  static std::random_device                     rd;
  static std::mt19937 generator(rd() + array.team().myid());

  dash::generate(array.begin(), array.end(), []() {
    return Point{distribution(generator), distribution(generator)};
  });

  array.barrier();

  dash::sort(array.begin(), array.end(), [](const Point& p) { return p.x; });

  if (dash::myid() == 0) {
    for (auto it = array.begin() + 1; it < array.end(); ++it) {
      auto const a = static_cast<const Element_t>(*(it - 1));
      auto const b = static_cast<const Element_t>(*it);

      EXPECT_FALSE_U(b < a);
    }
  }
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

      EXPECT_FALSE_U(b < a);
    }
  }
}

// TODO: add additional unit tests with various pattern types and containers

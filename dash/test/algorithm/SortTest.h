#ifndef DASH__TEST__SORT_TEST_H_
#define DASH__TEST__SORT_TEST_H_

#include "../TestBase.h"

/**
 * Test fixture for class dash::sort
 */
class SortTest : public dash::test::TestBase {
protected:
  size_t const num_local_elem = 100;

  SortTest()
  {
  }

  virtual ~SortTest()
  {
  }
};

struct Point {
  int32_t x;
  int32_t y;
};

constexpr bool operator<(const Point& lhs, const Point& rhs) noexcept
{
  return lhs.x < rhs.x;
}

std::ostream& operator<<(std::ostream& stream, const Point& p)
{
  stream << "{" << p.x << ", " << p.y << "}";
  return stream;
}

#endif  // DASH__TEST__SORT_TEST_H_

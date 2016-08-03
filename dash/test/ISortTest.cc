
#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "ISortTest.h"

#include <iostream>


TEST_F(ISortTest, ReverseOrder) {

  typedef int value_t;

  size_t nelem_per_unit = 10;
  dash::Array<value_t> array(dash::size() * nelem_per_unit);

  value_t min_key = 0;
  value_t max_key = nelem_per_unit * dash::size();

  for (auto i = 0; i < nelem_per_unit; i++) {
    array.local[i] = (dash::size() - dash::myid()) * nelem_per_unit - i;
  }

  if (dash::myid() == 0) {
    for (auto v : array) {
      std::cerr << " " << static_cast<value_t>(v);
    }
    std::cerr << std::endl;
  }

  std::cout << "Sorting array ... " << std::endl;
  dash::isort(array.begin(), array.end(), min_key, max_key);

  if (dash::myid() == 0) {
    for (auto v : array) {
      std::cerr << " " << static_cast<value_t>(v);
    }
    std::cerr << std::endl;
  }
}

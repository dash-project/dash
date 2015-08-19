#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "STLAlgorithmTest.h"

#include <algorithm>
#include <utility>

namespace std {
  template <class T1, class T2>
  std::ostream & operator<<(
    std::ostream & os,
    const std::pair<T1, T2> & p) {
    os << "(" << p.first << "," << p.second << ")";
    return os;
  }
}

TEST_F(STLAlgorithmTest, Copy) {
  typedef std::pair<dart_unit_t, int> element_t;
  typedef dash::Array<element_t>      array_t;
  typedef array_t::const_iterator     const_it_t;
  typedef array_t::index_type         index_t;
  size_t local_size = 50;
  dash::Array<element_t> array(_dash_size * local_size);
  // Initialize local elements
  index_t l_off = 0;
  for (auto l_it = array.lbegin(); l_it != array.lend(); ++l_it, ++l_off) {
    *l_it = std::make_pair(_dash_id, l_off);
  }
  // Global ranges to copy are dealt to units from the back
  // to ensure most ranges are copied global-to-local.
  auto global_offset      = array.size() - 
                            ((_dash_id + 1) * local_size);
  element_t * local_range = new element_t[local_size];
  const_it_t global_begin = array.begin() + global_offset;
  // Copy global element range to local memory
  std::copy(
    global_begin,
    global_begin + local_size,
    local_range);
  // Test and modify elements in local memory
  for (auto li = 0; li < local_size; ++li) {
    // Check if local copies are identical to global values
    element_t g_element = array[global_offset + li];
    element_t l_element = local_range[li];
    ASSERT_EQ_U(
      g_element,
      l_element);
    // Modify local copies
    local_range[li].first   = _dash_id;
    local_range[li].second += 1000;
  }
  // Copy modified local elements back to global array
  std::copy(
      local_range,
      local_range + local_size,
      array.begin() + global_offset);
  // Test elements in global array
  index_t i = 0;
  for (auto li = 0; li < local_size; ++li) {
    element_t g_element = array[global_offset + li];
    element_t l_element = local_range[li];
    // Plausibility checks of element
    ASSERT_EQ_U(g_element.first, _dash_id);
    ASSERT_EQ_U(g_element.second, 1000 + li);
    ASSERT_EQ_U(g_element, l_element);
  }
  // Free local memory
  delete[] local_range;
}

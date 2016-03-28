#include <unistd.h>
#include <iostream>
#include <libdash.h>
#include <algorithm>

using namespace std;

int main(int argc, char* argv[])
{
  typedef int                                         value_t;
  typedef typename dash::Array<value_t>::pattern_type pattern_t;
  typedef typename pattern_t::index_type              index_t;

  static const size_t NUM_ELEM_PER_UNIT = 10;

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  pattern_t pat(NUM_ELEM_PER_UNIT * size);

  // testing of various constructor options
  dash::Array<value_t> arr1(NUM_ELEM_PER_UNIT * size);
  dash::Array<value_t> arr2(NUM_ELEM_PER_UNIT * size, dash::BLOCKED);
  dash::Array<value_t> arr3(NUM_ELEM_PER_UNIT * size,
                            dash::Team::All());
  dash::Array<value_t> arr4(NUM_ELEM_PER_UNIT * size, dash::BLOCKED,
			                      dash::Team::All());
  dash::Array<value_t> arr5(pat);

  if (myid == 0) {
    for (auto i = 0; i < arr1.size(); i++) {
      arr1[i] = i;
      arr2[i] = i;
      arr3[i] = i;
      arr4[i] = i;
      arr5[i] = i;
    }
  }

  dash::Team::All().barrier();

  if (myid == size-1) {
    for (auto i = 0; i < arr1.size(); i++) {
      assert(i == arr1[i]);
      assert(static_cast<value_t>(arr1[i]) == static_cast<value_t>(arr2[i]));
      assert(static_cast<value_t>(arr1[i]) == static_cast<value_t>(arr3[i]));
      assert(static_cast<value_t>(arr1[i]) == static_cast<value_t>(arr4[i]));
      assert(static_cast<value_t>(arr1[i]) == static_cast<value_t>(arr5[i]));
    }
    for (auto el: arr1) {
      cout << static_cast<value_t>(el)
           << " ";
    }
    cout << endl;
  }

  dash::finalize();
}

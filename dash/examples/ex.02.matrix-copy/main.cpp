
#include <libdash.h>

#include <vector>
#include <iostream>

using std::cout;
using std::endl;


int main(int argc, char **argv)
{
  constexpr int elem_per_unit = 1000;
  dash::init(&argc, &argv);

  std::vector<double> vec(100);
  std::fill(vec.begin(), vec.end(), dash::myid());

  dash::NArray<double, 2> matrix;

  dash::SizeSpec<2> sizespec(dash::size(), elem_per_unit);
  dash::DistributionSpec<2> distspec(dash::BLOCKED, dash::NONE);
  dash::TeamSpec<2> teamspec(dash::size(), 1);

  matrix.allocate(sizespec, distspec, teamspec);

  if (dash::myid() == 0) {
    // ostream<< overload for iterators is broken
    // std::cout << matrix.row(1).begin() << std::endl;
//  dash::copy(&vec[0], &vec[0] + vec.size(), matrix.row(1).begin() + 100);

    auto dest_range = dash::sub<0>(
                        1,2, matrix);

    using globiter_t = decltype(matrix.begin());

    cout << "matrix.row(1).begin() + 100: "
         << (matrix.row(1).begin() + 100)
         << endl;
    cout << "sub... expression:           "
         << (dest_range.begin() + 100)
         << endl;
    cout << "view expr. cast to gptr:     "
         << static_cast<globiter_t>(dest_range.begin() + 100)
         << endl;

    dash::copy(&vec[0],
               &vec[0] + vec.size(),
               static_cast<globiter_t>(dest_range.begin()));
  }

  matrix.barrier();
  dash::finalize();

  return 0;
}

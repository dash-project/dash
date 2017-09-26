
#include <libdash.h>
#include "../util.h"

#include <vector>
#include <iostream>

using std::cout;
using std::endl;

using namespace dash;

int main(int argc, char **argv)
{
  using dash::sub;
  using dash::local;
  using dash::index;
  using dash::blocks;

  typedef dash::TilePattern<2>           pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  int blocksize_x = 2;
  int blocksize_y = 3;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  dash::SizeSpec<2>         sizespec((nunits * 2) * blocksize_y,
                                     (nunits * 2) * blocksize_x);
  dash::DistributionSpec<2> distspec(dash::BLOCKED, // dash::TILE(blocksize_y),
                                     dash::BLOCKED);//dash::TILE(blocksize_x));
  dash::TeamSpec<2>         teamspec(dash::Team::All());
  teamspec.balance_extents();

  pattern_t pattern(
    sizespec,
    distspec,
    teamspec
  );

  dash::Matrix<value_t, 2, index_t, pattern_t> matrix(pattern);

  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });

  dash::barrier();

  if (dash::myid() == 0) {
    print("matrix:" <<
          nview_str(matrix | sub(0,matrix.extents()[0])));
  }

  dash::barrier();

  print("matrix.local size: " << matrix.local.size());

  for (int li = 0; li < matrix.local.size(); ++li) {
    DASH_LOG_DEBUG_VAR("matrix.local", li);

    auto lit = matrix.local.begin() + li;

    DASH_LOG_DEBUG_VAR("matrix.local", lit);
    print("matrix.local[" << li << "]: " << *lit);
  }

  auto l_row = matrix.local.row(0);
  print("matrix.local.row(0) " <<
        "size: "    << l_row.size() << " "
        "offsets: " << l_row.offsets() << " "
        "extents: " << l_row.extents());

  auto l_row_range = dash::make_range(l_row.begin(), l_row.end());
  print("matrix.local.row(0) range type: " << dash::typestr(l_row_range));
  print("matrix.local.row(0) range: " << l_row_range);

  std::vector<value_t> tmp(l_row.size());
  auto copy_end = dash::copy(l_row.begin(), l_row.end(),
                             tmp.data());

  matrix.barrier();
  dash::finalize();

  return 0;
}

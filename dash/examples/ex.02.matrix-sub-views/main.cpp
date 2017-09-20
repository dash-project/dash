#include <libdash.h>
#include "../util.h"

using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::vector;

using uint = unsigned int;


int main(int argc, char *argv[])
{
  using namespace dash;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 3;
  const size_t block_size_y  = 3;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * nunits - 1;
  size_t extent_y            = block_size_y * nunits - 1;

  if (nunits < 2) {
    cerr << "requires > 1 units" << endl;
    return 1;
  }

  typedef dash::ShiftTilePattern<2>      pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_y,
      extent_x),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_y),
      dash::TILE(block_size_x))
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix(pattern);

  // Initialize matrix values:
  int li = 0;
  std::generate(matrix.lbegin(),
                matrix.lend(),
                [&]() { return dash::myid() + 0.01 * li++; });
  dash::barrier();

  if (myid == 0) {
    print("matrix:" <<
          nview_str(matrix | sub(0,extent_y)) << '\n');
    print("matrix | sub<0>(2,-2) | sub<1>(2,-3):" <<
          nview_str(matrix | sub<0>(2, extent_y-2)
                           | sub<1>(2, extent_x-3)) << '\n');

    auto m_blocks = matrix | sub<0>(2, extent_y-2)
                           | sub<1>(2, extent_x-3)
                           | blocks();
    for (const auto & m_block : m_blocks) {
      print("matrix | sub<0>(2,-2) | sub<1>(2,-3) | blocks():" <<
            nview_str(m_block) << '\n');
    }
  }
  dash::barrier();

  dash::finalize();
  return EXIT_SUCCESS;
};

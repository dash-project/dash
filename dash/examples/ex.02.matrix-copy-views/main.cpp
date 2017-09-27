#include <libdash.h>
#include "../util.h"


int main(int argc, char *argv[])
{
  using namespace dash;

  typedef dash::SeqTilePattern<2>          pattern_t;
  typedef typename pattern_t::index_type   index_t;
  typedef float                            value_t;

  dash::init(&argc, &argv);

  auto myid   = dash::Team::All().myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 2;
  const size_t block_size    = block_size_x * block_size_y;
  size_t extent_x            = block_size_x * (nunits + 1);
  size_t extent_y            = block_size_y * (nunits + 1);

  dash::TeamSpec<2> teamspec(dash::Team::All());
  teamspec.balance_extents();

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_y,
      extent_x),
    dash::DistributionSpec<2>(
      dash::TILE(block_size_y),
      dash::TILE(block_size_x)),
    teamspec
  );

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix_a(pattern);

  dash::Matrix<value_t, 2, dash::default_index_t, pattern_t>
    matrix_b(pattern);

  int li;

  li = 0;
  std::generate(matrix_a.lbegin(),
                matrix_a.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });
  li = 0;
  std::generate(matrix_b.lbegin(),
                matrix_b.lend(),
                [&]() {
                  return dash::myid() + 0.01 * li++;
                });
  dash::barrier();

  if (myid == 0) {
    print("matrix_a:" <<
          nview_str(matrix_a | sub(0,extent_y)) << '\n');
    print("matrix_b:" <<
          nview_str(matrix_b | sub(0,extent_y)) << '\n');

    print("matrix number of blocks:" <<(matrix_b | blocks()).size());
  }
  dash::barrier();

  index_t n_lblocks = (matrix_b | local() | blocks()).size();
  print("matrix number of local blocks:" <<(matrix_b | blocks()).size());


  index_t dst_block = dash::myid() + 1;
  
  auto copy_end = dash::copy(matrix_a | local() | block(1),
                             matrix_b | block(dst_block));

  if (myid == 0) {
    print("matrix:" <<
          nview_str(matrix_b | sub(0,extent_y)) << '\n');

    print("matrix number of blocks:" <<(matrix_b | blocks()).size());
  }


  dash::finalize();
  return EXIT_SUCCESS;
}

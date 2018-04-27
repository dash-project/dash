#include <libdash.h>
#include "../util.h"

using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::vector;

using namespace dash;

using uint = unsigned int;

template <typename MatrixT, typename ViewMods>
auto transform_in_view(MatrixT & mat, ViewMods && vmods) {
  print("transform_in_view: " << nview_str(mat | vmods));
  return 0;
}


int main(int argc, char *argv[])
{
  using dash::sub;
  using dash::local;
  using dash::index;
  using dash::blocks;

  dash::init(&argc, &argv);

  auto myid   = dash::myid();
  auto nunits = dash::size();

  const size_t block_size_x  = 2;
  const size_t block_size_y  = 3;
  const size_t block_size    = block_size_x * block_size_y;
  size_t num_local_blocks_x  = 2;
  size_t num_local_blocks_y  = 2;
  size_t num_blocks_x        = nunits * num_local_blocks_x;
  size_t num_blocks_y        = nunits * num_local_blocks_y;
  size_t num_blocks_total    = num_blocks_x * num_blocks_y;
  size_t extent_x            = block_size_x * num_blocks_x;
  size_t extent_y            = block_size_y * num_blocks_y;
  size_t num_elem_total      = extent_x * extent_y;
  // Assuming balanced mapping:
  size_t num_elem_per_unit   = num_elem_total / nunits;
  size_t num_blocks_per_unit = num_elem_per_unit / block_size;


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
                [&]() {
                  auto   u = dash::myid();
                  return u + 0.01 * li++;
                });
  dash::barrier();

  if (myid == 0) {
    print("matrix:" <<
          nview_str(dash::sub(0, matrix.extents()[0], matrix)));

    transform_in_view(matrix, sub<0>(2,4) | sub<1>(2,6));

    auto matrix_region = dash::size() > 1
                         ? dash::sub<0>(
                             2, matrix.extents()[0] - 2,
                             dash::sub<1>(
                               2, matrix.extents()[1] - 3,
                               matrix))
                         : dash::sub<0>(
                             0, matrix.extents()[0],
                             dash::sub<1>(
                               0, matrix.extents()[1],
                               matrix));

    print("matrix | sub<0>(2,-2) | sub<1>(2,-3) \n" <<
          nview_str(matrix_region));

    auto matrix_reg_blocks = matrix_region | blocks();
    int bi = 0;
    for (const auto & reg_block : matrix_reg_blocks) {
      print("matrix | sub | sub | block[" << bi << "] " <<
            "extents: " << reg_block.extents() << " " <<
            "offsets: " << reg_block.offsets());
      print(nview_str(reg_block) << '\n');

  //  auto sreg_block = reg_block | dash::intersect(
  //                                  ViewSpec<2>({ 1,2 }));
  //  if (!sreg_block) { continue; }
  //  DASH_LOG_DEBUG("MatrixViewsExample", sreg_block.size());
  //  DASH_LOG_DEBUG("MatrixViewsExample",
  //                 dash::typestr(sreg_block.begin()));
  //  DASH_LOG_DEBUG("MatrixViewsExample",
  //                 nview_str(sreg_block));
      ++bi;
    }
  }
  dash::barrier();

  // Array to store local copy:
  std::vector<value_t> local_copy(num_elem_per_unit);
  // Pointer to first value in next copy destination range:
  value_t * copy_dest_begin = local_copy.data();
  value_t * copy_dest_last  = local_copy.data();

  print("Number of blocks: " << num_blocks_total);

  for (size_t gb = 0; gb < num_blocks_total; ++gb) {
    // View of block at global block index gb:
    auto g_block_view = pattern.block(gb);
    // Unit assigned to block at global block index gb:
    auto g_block_unit = pattern.unit_at(
                          std::array<index_t, 2> {0,0},
                          g_block_view);
    dash::team_unit_t remote_unit_id(
                         (dash::Team::All().myid().id + 1) % nunits);
    if (g_block_unit == remote_unit_id) {
      DASH_LOG_DEBUG("MatrixViewsExample", "===========================");
      print("--- block gidx " << gb << " at unit " << g_block_unit.id);

      DASH_LOG_DEBUG("MatrixViewsExample", "vvvvvvvvvvvvvvvvvvvvvvvvvvv");
      // Block is assigned to selecte remote unit, create local copy:
      auto remote_block_matrix = dash::sub(1,5, matrix.block(gb));

      auto remote_block_view   = dash::sub(1,5, dash::blocks(matrix)[gb]);

      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     dash::typestr(remote_block_view));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block view iterator:",
                     dash::typestr(remote_block_view.begin()));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block view domain:",
                     dash::typestr(dash::domain(remote_block_view)));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block view origin:",
                     dash::typestr(dash::origin(remote_block_view)));

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view:",
                     "extents:", remote_block_view.extents(),
                     "offsets:", remote_block_view.offsets(),
                     "size:",    remote_block_view.size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view domain:",
                     "extents:", dash::domain(remote_block_view).extents(),
                     "offsets:", dash::domain(remote_block_view).offsets(),
                     "size:",    dash::domain(remote_block_view).size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "begin.pos:",  remote_block_view.begin().pos(),
                     "end.pos:",    remote_block_view.end().pos(),
                     "begin.gpos:", remote_block_view.begin().gpos(),
                     "end.gpos:",   remote_block_view.end().gpos());
      DASH_LOG_DEBUG("MatrixViewsExample", "block view:",
                     nview_str(remote_block_view));

      DASH_LOG_DEBUG("MatrixViewsExample", "block view index type:",
                     dash::typestr(
                       dash::index(remote_block_view)));
      DASH_LOG_DEBUG("MatrixViewsExample", "block view index is strided:",
                     dash::index(remote_block_view)
                       .is_strided());

      DASH_LOG_DEBUG("MatrixViewsExample", "local block view index type:",
                     dash::typestr(
                       dash::index(dash::local(remote_block_view))));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "local block view index is strided:",
                     dash::index(dash::local(remote_block_view))
                       .is_strided());

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "local block view index domain type:",
                     dash::typestr(
                       dash::domain(
                         dash::index(dash::local(remote_block_view)))));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "local block view index domain is strided:",
                     dash::domain(
                       dash::index(dash::local(remote_block_view)))
                         .is_strided());

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "local block view index set size:",
                     dash::index(dash::local(remote_block_view))
                       .size());

      DASH_ASSERT(remote_block_matrix.offsets() ==
                  dash::index(remote_block_view).offsets());
      DASH_ASSERT(remote_block_matrix.extents() ==
                  dash::index(remote_block_view).extents());

      copy_dest_last  = dash::copy(remote_block_view,
                                   copy_dest_begin);

      // Test number of copied elements:
      auto num_copied = copy_dest_last - copy_dest_begin;
      DASH_ASSERT(num_copied == block_size);

      // Advance local copy destination pointer:
      copy_dest_begin = copy_dest_last;

      DASH_LOG_DEBUG("MatrixViewsExample", "^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
  }
  print("local copy of all remote values:\n" << local_copy);

  dash::finalize();
  return EXIT_SUCCESS;
}

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

    auto matrix_reg_blocks = dash::blocks(matrix_region);
    for (const auto & reg_block : matrix_reg_blocks) {
      auto sreg_block = dash::sub<0>(1,2, reg_block);

      DASH_LOG_DEBUG("MatrixViewsExample", "==============================",
                     nview_str(reg_block));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     dash::typestr(sreg_block.begin()));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     nview_str(sreg_block));

      auto block_rg  = dash::make_range(reg_block.begin(),
                                        reg_block.end());
      auto block_srg = dash::sub<0>(1,2, block_rg);

      DASH_LOG_DEBUG("MatrixViewsExample", "------------------------------",
                     nview_str(block_rg));
      DASH_LOG_DEBUG("MatrixViewsExample", "block range origin iterator:",
                     dash::typestr(dash::origin(block_srg).begin()));
   // DASH_LOG_DEBUG("MatrixViewsExample", "block range origin:",
   //                nview_str(dash::origin(block_srg)));
   // DASH_LOG_DEBUG("MatrixViewsExample",
   //                nview_str(block_srg));
    }
  }
  dash::barrier();

  // Array to store local copy:
  std::vector<value_t> local_copy(num_elem_per_unit);
  // Pointer to first value in next copy destination range:
  value_t * copy_dest_begin = local_copy.data();
  value_t * copy_dest_last  = local_copy.data();

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
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "block gidx", gb,
                     "at unit",    g_block_unit.id);
      DASH_LOG_DEBUG("MatrixViewsExample", "vvvvvvvvvvvvvvvvvvvvvvvvvvv");
      // Block is assigned to selecte remote unit, create local copy:
      auto remote_block_matrix = matrix.block(gb);

      auto remote_block_view   = dash::blocks(matrix)[gb];

      // Test number of copied elements:
      auto num_copied = copy_dest_last - copy_dest_begin;
      DASH_ASSERT(num_copied == block_size);

      // Advance local copy destination pointer:
      copy_dest_begin = copy_dest_last;

      auto remote_block_range  = dash::sub(1,3,
                                  dash::make_range(
                                   remote_block_view.begin(),
                                   remote_block_view.end()));

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range:", "-- type:",
                     dash::typestr(remote_block_range));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block range iterator:",
                     dash::typestr(remote_block_range.begin()));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block range domain:",
                     dash::typestr(dash::domain(remote_block_range)));
      DASH_LOG_DEBUG("MatrixViewsExample", "-- type:",
                     "source block range origin:",
                     dash::typestr(dash::origin(remote_block_range)));

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range:",
                     "extents:", remote_block_range.extents(),
                     "offsets:", remote_block_range.offsets(),
                     "size:",    remote_block_range.size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range domain:",
                     "extents:", dash::domain(remote_block_range).extents(),
                     "offsets:", dash::domain(remote_block_range).offsets(),
                     "size:",    dash::domain(remote_block_range).size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "begin.pos:",  remote_block_range.begin().pos(),
                     "end.pos:",    remote_block_range.end().pos(),
                     "begin.gpos:", remote_block_range.begin().gpos(),
                     "end.gpos:",   remote_block_range.end().gpos());
      DASH_LOG_DEBUG("MatrixViewsExample", "block range index:",
                     nview_str(dash::index(remote_block_range)));
      DASH_LOG_DEBUG("MatrixViewsExample", "block range index is strided:",
                     dash::index(remote_block_range).is_strided());
      DASH_LOG_DEBUG("MatrixViewsExample", "block range:",
                     nview_str(remote_block_range));
      DASH_LOG_DEBUG("MatrixViewsExample", "local(block range):",
                     nview_str(dash::local(remote_block_range)));

      copy_dest_last  = dash::copy(remote_block_range,
                                   copy_dest_begin);

      DASH_LOG_DEBUG("MatrixViewsExample", "^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
  }
  print("local copy of all remote values:\n" << local_copy);

  dash::finalize();
  return EXIT_SUCCESS;
}

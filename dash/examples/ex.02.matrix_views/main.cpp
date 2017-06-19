#include <libdash.h>

using std::cout;
using std::cerr;
using std::cin;
using std::endl;
using std::vector;

using uint = unsigned int;

template <class NViewType>
std::string nview_str(
  const NViewType   & nview) {
  using value_t   = typename NViewType::value_type;
  auto view_nrows = nview.extents()[0];
  auto view_ncols = nview.extents()[1];
  auto nindex     = dash::index(nview);
  std::ostringstream ss;
  for (int r = 0; r < view_nrows; ++r) {
    ss << '\n';
    for (int c = 0; c < view_ncols; ++c) {
      int offset = r * view_ncols + c;
      ss << std::fixed << std::setw(3)
         << nindex[offset]
         << ":"
         << std::fixed << std::setprecision(5)
         << static_cast<const value_t>(nview[offset])
         << " ";
    }
  }
  return ss.str();
}

inline void sum(const uint nelts,
                const dash::NArray<uint, 2> &matIn,
                const uint myid) {
  uint lclRows = matIn.pattern().local_extents()[0];

  uint const *mPtr;
  uint localSum = 0;

  for (uint i = 0; i < lclRows; ++i) {
    mPtr = matIn.local.row(i).lbegin();

    for (uint j = 0; j < nelts; ++j) {
      localSum += *(mPtr++);
    }
  }
}

int main(int argc, char *argv[])
{
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

  if (nunits < 2) {
    cerr << "requires > 1 units" << endl;
    return 1;
  }

  typedef dash::ShiftTilePattern<2>      pattern_t;
  typedef typename pattern_t::index_type index_t;
  typedef float                          value_t;

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
  for (size_t lb = 0; lb < num_blocks_per_unit; ++lb) {
    auto lblock         = matrix.local.block(lb);
    auto lblock_view    = lblock.begin().viewspec();
    auto lblock_extents = lblock_view.extents();
    auto lblock_offsets = lblock_view.offsets();
    dash__unused(lblock_offsets);
    for (auto bx = 0; bx < static_cast<int>(lblock_extents[0]); ++bx) {
      for (auto by = 0; by < static_cast<int>(lblock_extents[1]); ++by) {
        // Phase coordinates (bx,by) to global coordinates (gx,gy):
        value_t value  = static_cast<value_t>(dash::myid().id + 1) +
                           0.00001 * (
                             ((lb + 1) * 10000) + ((bx + 1) * 100) + by + 1);
        lblock[bx][by] = value;
      }
    }
  }
  dash::barrier();

  if (myid == 0) {
    DASH_LOG_DEBUG("MatrixViewsExample",
                   nview_str(dash::sub(0, matrix.extents()[0], matrix)));
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

      auto remote_block_view   = // dash::sub(1,2,
                                           dash::blocks(matrix)[gb];
      auto remote_block_range  = dash::make_range(
                                   remote_block_view.begin(),
                                   remote_block_view.end());

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view:",
                     dash::typestr(remote_block_view));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view",
                     "extents:", remote_block_view.extents(),
                     "offsets:", remote_block_view.offsets(),
                     "size:",    remote_block_view.size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view domain:",
                     dash::typestr(dash::domain(remote_block_view)));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view origin:",
                     dash::typestr(dash::origin(remote_block_view)));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view domain extents:",
                     dash::domain(remote_block_view).extents());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block view iterator:",
                     dash::typestr(remote_block_view.begin()));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "begin.pos:",  remote_block_view.begin().pos(),
                     "end.pos:",    remote_block_view.end().pos(),
                     "begin.gpos:", remote_block_view.begin().gpos(),
                     "end.gpos:",   remote_block_view.end().gpos());
      DASH_LOG_DEBUG("MatrixViewsExample", "block view:",
                     nview_str(remote_block_view));
      DASH_LOG_DEBUG("MatrixViewsExample", "local(block view):",
                     nview_str(dash::local(remote_block_view)));

      DASH_ASSERT(remote_block_matrix.viewspec().offsets() ==
                  dash::index(remote_block_view).offsets());
      DASH_ASSERT(remote_block_matrix.viewspec().extents() ==
                  dash::index(remote_block_view).extents());

      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range:",
                     dash::typestr(remote_block_range));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range",
                     "extents:", remote_block_range.extents(),
                     "offsets:", remote_block_range.offsets(),
                     "size:",    remote_block_range.size());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range domain:",
                     dash::typestr(dash::domain(remote_block_range)));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range origin:",
                     dash::typestr(dash::origin(remote_block_range)));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range domain extents:",
                     dash::domain(remote_block_range).extents());
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "source block range iterator:",
                     dash::typestr(remote_block_range.begin()));
      DASH_LOG_DEBUG("MatrixViewsExample",
                     "begin.pos:",  remote_block_range.begin().pos(),
                     "end.pos:",    remote_block_range.end().pos(),
                     "begin.gpos:", remote_block_range.begin().gpos(),
                     "end.gpos:",   remote_block_range.end().gpos());
      DASH_LOG_DEBUG("MatrixViewsExample", "block range:",
                     nview_str(remote_block_range));
      DASH_LOG_DEBUG("MatrixViewsExample", "local(block range):",
                     nview_str(dash::local(remote_block_range)));
#if 0
      copy_dest_last  = dash::copy(remote_block_view.begin(),
                                   remote_block_view.end(),
                                   copy_dest_begin);

      // Test number of copied elements:
      auto num_copied = copy_dest_last - copy_dest_begin;
      DASH_ASSERT(num_copied == block_size);

      // Advance local copy destination pointer:
      copy_dest_begin = copy_dest_last;
#endif
      DASH_LOG_DEBUG("MatrixViewsExample", "^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    }
  }

  dash::finalize();
}

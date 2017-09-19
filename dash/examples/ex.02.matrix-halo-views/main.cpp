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
         << static_cast<value_t>(nview[offset])
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

  typedef dash::SeqTilePattern<2>         pattern_t;
  typedef typename pattern_t::index_type    index_t;
  typedef float                             value_t;

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
    auto matrix_view = dash::sub(0, matrix.extents()[0], matrix);
    DASH_LOG_DEBUG("MatrixViewsExample",
                   nview_str(matrix_view));
      DASH_LOG_DEBUG("MatrixViewsExample", "matrix",
                     "offsets:", matrix_view.offsets(),
                     "extents:", matrix_view.extents());

    auto matrix_blocks = dash::blocks(matrix_view);
    for (const auto & m_block : matrix_blocks) {
      DASH_LOG_DEBUG("MatrixViewsExample", "m_block ======================",
                     nview_str(m_block));
      // halo view:
      auto b_halo = dash::sub<0>(
                      m_block.offsets()[0] > 0
                        ? m_block.offsets()[0] -1
                        : m_block.offsets()[0],
                      m_block.offsets()[0] + m_block.extents()[0]
                          < matrix_view.extents()[0]
                        ? m_block.offsets()[0] + m_block.extents()[0] + 1
                        : m_block.offsets()[0] + m_block.extents()[0],
                      dash::sub<1>(
                        m_block.offsets()[1] > 0
                          ? m_block.offsets()[1] -1
                          : m_block.offsets()[1],
                        m_block.offsets()[1] + m_block.extents()[1]
                            < matrix_view.extents()[1]
                          ? m_block.offsets()[1] + m_block.extents()[1] + 1
                          : m_block.offsets()[1] + m_block.extents()[1],
                        matrix_view));
      DASH_LOG_DEBUG("MatrixViewsExample", "b_halo -----------------------",
                     nview_str(b_halo));
      DASH_LOG_DEBUG("MatrixViewsExample", "size:", b_halo.size());
#if 0
      auto b_halo_isect = dash::difference(
                            m_block,
                            dash::sub<0>(
                              1, m_block.extents()[0] - 1,
                              dash::sub<1>(
                                1, m_block.extents()[1] - 1,
                                m_block)));

      auto b_halo_exp = dash::expand<0>(
                          -1, 1,
                          dash::expand<1>(
                            -1, 1,
                            m_block));
      DASH_LOG_DEBUG("MatrixViewsExample", "------------------------------",
                     nview_str(b_halo_exp));
#endif
    }
  }
  dash::barrier();

  dash::finalize();
  return EXIT_SUCCESS;
}

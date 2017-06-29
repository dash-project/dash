
#include "NViewTest.h"

#include <gtest/gtest.h>

#include <dash/View.h>
#include <dash/Array.h>
#include <dash/Matrix.h>
#include <dash/Meta.h>
#include <dash/Pattern.h>

#include <array>
#include <string>
#include <sstream>
#include <iomanip>


namespace dash {
namespace test {

  template <class MatrixT>
  void initialize_matrix(MatrixT & matrix) {
    if (dash::myid() == 0) {
      for(size_t i = 0; i < matrix.extent(0); ++i) {
        for(size_t k = 0; k < matrix.extent(1); ++k) {
          matrix[i][k] = (i + 1) * 0.100 + (k + 1) * 0.001;
        }
      }
    }
    matrix.barrier();

    for(size_t i = 0; i < matrix.local_size(); ++i) {
      matrix.lbegin()[i] += dash::myid();
    }
    matrix.barrier();
  }

  template <class NViewType>
  void print_nview(
    const std::string & name,
    const NViewType   & nview) {
    using value_t   = typename NViewType::value_type;
    auto view_nrows = nview.extents()[0];
    auto view_ncols = nview.extents()[1];
    auto nindex     = dash::index(nview);
    for (int r = 0; r < view_nrows; ++r) {
      std::ostringstream row_ss;
      for (int c = 0; c < view_ncols; ++c) {
        int offset = r * view_ncols + c;
        row_ss << std::fixed << std::setw(2)
               << nindex[offset]
               << ":"
               << std::fixed << std::setprecision(3)
               << static_cast<value_t>(nview[offset])
               << " ";
      }
      DASH_LOG_DEBUG("NViewTest.print_nview",
                     name, "[", r, "]", row_ss.str());
    }
  }

  template <class NViewType>
  std::vector<typename NViewType::value_type>
  region_values(const NViewType & view, const dash::ViewSpec<2> & vs) {
    auto nvalues = vs.size();
    using value_t = typename NViewType::value_type;
    std::vector<value_t> values;
    values.reserve(nvalues);
    dash::CartesianIndexSpace<2> cart(view.extents());
    for (int i = 0; i < nvalues; i++) {
      auto coords = cart.coords(i, vs);
      auto index  = cart.at(coords);
      values.push_back(static_cast<value_t>(view.begin()[index]));
    }
    return values;
  }
}
}

using dash::test::range_str;

TEST_F(NViewTest, ViewTraits)
{
  {
    dash::Matrix<int, 2> matrix(
        dash::SizeSpec<2>(
          dash::size() * 10,
          dash::size() * 10),
        dash::DistributionSpec<2>(
          dash::NONE,
          dash::TILE(10)),
        dash::Team::All(),
        dash::TeamSpec<2>(
          1,
          dash::size()));

    auto v_sub  = dash::sub<0>(0, 10, matrix);
    auto i_sub  = dash::index(v_sub);
    auto v_ssub = dash::sub<0>(0, 5, (dash::sub<1>(0, 10, matrix)));
    auto v_loc  = dash::local(matrix);
    auto v_lsub = dash::local(dash::sub<1>(0, 10, matrix));
    auto v_sblk = dash::blocks(dash::sub<0>(0, 10, matrix));

    static_assert(
        dash::view_traits<decltype(matrix)>::rank::value == 2,
        "view traits rank for dash::Matrix not matched");
    static_assert(
        dash::view_traits<decltype(v_sblk)>::rank::value == 2,
        "view traits rank for blocks(sub(dash::Matrix)) not matched");

    static_assert(
        dash::view_traits<decltype(v_sub)>::is_view::value == true,
        "view traits is_view for sub(dash::Matrix) not matched");
    static_assert(
        dash::view_traits<decltype(v_ssub)>::is_view::value == true,
        "view traits is_view for sub(sub(dash::Matrix)) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_view::value == true,
        "view traits is_view for local(sub(dash::Matrix)) not matched");

    static_assert(
        dash::view_traits<decltype(v_loc)>::is_origin::value == true,
        "view traits is_origin for local(dash::Matrix) not matched");
    static_assert(
        dash::view_traits<decltype(v_sub)>::is_origin::value == false,
        "view traits is_origin for sub(dash::Matrix) not matched");
    static_assert(
        dash::view_traits<decltype(v_ssub)>::is_origin::value == false,
        "view traits is_origin for sub(sub(dash::Matrix)) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_origin::value == false,
        "view traits is_origin for local(sub(dash::Matrix)) not matched");

    static_assert(
        dash::view_traits<decltype(v_loc)>::is_local::value == true,
        "view traits is_local for local(dash::Matrix) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_local::value == true,
        "view traits is_local for local(sub(dash::Matrix)) not matched");
  }
  {
    dash::NArray<int, 2> narray(
        dash::SizeSpec<2>(
          dash::size() * 10,
          dash::size() * 10),
        dash::DistributionSpec<2>(
          dash::NONE,
          dash::BLOCKCYCLIC(10)),
        dash::Team::All(),
        dash::TeamSpec<2>(
          1,
          dash::size()));

    auto v_sub  = dash::sub<0>(0, 10, narray);
    auto i_sub  = dash::index(v_sub);
    auto v_ssub = dash::sub<0>(0, 5, (dash::sub<1>(0, 10, narray)));
    auto v_loc  = dash::local(narray);
    auto v_lsub = dash::local(dash::sub<1>(0, 10, narray));
    auto v_sblk = dash::blocks(dash::sub<0>(0, 10, narray));

    static_assert(
        dash::view_traits<decltype(narray)>::rank::value == 2,
        "view traits rank for dash::NArray not matched");
    static_assert(
        dash::view_traits<decltype(v_sblk)>::rank::value == 2,
        "view traits rank for blocks(sub(dash::NArray)) not matched");

    static_assert(
        dash::view_traits<decltype(v_sub)>::is_view::value == true,
        "view traits is_view for sub(dash::NArray) not matched");
    static_assert(
        dash::view_traits<decltype(v_ssub)>::is_view::value == true,
        "view traits is_view for sub(sub(dash::NArray)) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_view::value == true,
        "view traits is_view for local(sub(dash::NArray)) not matched");

    static_assert(
        dash::view_traits<decltype(v_loc)>::is_origin::value == true,
        "view traits is_origin for local(dash::NArray) not matched");
    static_assert(
        dash::view_traits<decltype(v_sub)>::is_origin::value == false,
        "view traits is_origin for sub(dash::NArray) not matched");
    static_assert(
        dash::view_traits<decltype(v_ssub)>::is_origin::value == false,
        "view traits is_origin for sub(sub(dash::NArray)) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_origin::value == false,
        "view traits is_origin for local(sub(dash::NArray)) not matched");

    static_assert(
        dash::view_traits<decltype(v_loc)>::is_local::value == true,
        "view traits is_local for local(dash::NArray) not matched");
    static_assert(
        dash::view_traits<decltype(v_lsub)>::is_local::value == true,
        "view traits is_local for local(sub(dash::NArray)) not matched");
  }
}

TEST_F(NViewTest, MatrixBlocked1DimSingle)
{
  auto nunits = dash::size();

  int block_rows = 3;
  int block_cols = 4;

  if (nunits < 2) { block_cols = 8; }

  int nrows = 2      * block_rows;
  int ncols = nunits * block_cols;

  // columns distributed in blocks of same size:
  //
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //
  dash::Matrix<double, 2> mat(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::NONE,
        dash::TILE(block_cols)),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "Matrix initialized");

  if (dash::myid() == 0) {
    dash::test::print_nview("matrix", dash::sub<0>(0, mat.extent(0), mat));
  }
  mat.barrier();

  // select first 2 matrix rows:
  auto nview_total  = dash::sub<0>(0, mat.extent(0), mat);
  auto nview_local  = dash::local(nview_total);
  auto nview_rows_g = dash::sub<0>(1, 3, mat);
  auto nview_cols_g = dash::sub<1>(2, 7, mat);

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                   "mat ->",
                   "offsets:", mat.offsets(),
                   "extents:", mat.extents(),
                   "size:",    mat.size());

    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                   "sub<0>(1,3, mat) ->",
                   "offsets:", nview_rows_g.offsets(),
                   "extents:", nview_rows_g.extents(),
                   "size:",    nview_rows_g.size());
    dash::test::print_nview("nview_rows_g", nview_rows_g);

    auto exp_nview_rows_g = dash::test::region_values(
                              mat, {{ 1,0 }, { 2,mat.extent(1) }}
                            );

    EXPECT_TRUE(
      dash::test::expect_range_values_equal<double>(
        exp_nview_rows_g, nview_rows_g));

    EXPECT_EQ(2,             nview_rows_g.extent<0>());
    EXPECT_EQ(mat.extent(1), nview_rows_g.extent<1>());

    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                   "sub<1>(2,7, mat) ->",
                   "offsets:", nview_cols_g.offsets(),
                   "extents:", nview_cols_g.extents(),
                   "size:",    nview_cols_g.size(),
                   "strided:", dash::index(nview_cols_g).is_strided());
    dash::test::print_nview("nview_cols_g", nview_cols_g);

    auto exp_nview_cols_g = dash::test::region_values(
                              mat, {{ 0,2 }, { mat.extent(0),5 }} );
    EXPECT_TRUE_U(
      dash::test::expect_range_values_equal<double>(
        exp_nview_cols_g, nview_cols_g));

    EXPECT_EQ_U(mat.extent(0), nview_cols_g.extent<0>());
    EXPECT_EQ_U(5,             nview_cols_g.extent<1>());
  }

  mat.barrier();

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSingle",
                     mat.local_size());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSingle",
                     mat.pattern().local_size());
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "local(mat) ->",
                 dash::typestr(nview_local));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "local(mat) ->",
                 "it:",       dash::typestr(nview_local.begin()));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "local(mat) ->",
                 "offsets:",  nview_local.offsets(),
                 "extents:",  nview_local.extents(),
                 "size:",     nview_local.size());
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "index(local(mat)) ->",
                 "strided:",  dash::index(nview_local).is_strided(),
                 "pat.lbeg:", dash::index(nview_local).pattern().lbegin(),
                 "pat.lend:", dash::index(nview_local).pattern().lend(),
                 "distance:", dash::distance(nview_local.begin(),
                                             nview_local.end()));
  dash::test::print_nview("nview_local", nview_local);

  mat.barrier();

  auto nview_cols_l = dash::sub<1>(2,4, dash::local(dash::sub<0>(0,6, mat)));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "cols(local(mat)) ->",
                 dash::typestr(nview_cols_l));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "cols(local(mat)) ->",
                 "it:",       dash::typestr(nview_cols_l.begin()));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "cols(local(mat)) ->",
                 "offsets:",  nview_cols_l.offsets(),
                 "extents:",  nview_cols_l.extents(),
                 "size:",     nview_cols_l.size());
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "index(cols(local(mat))) ->",
                 "strided:",  dash::index(nview_cols_l).is_strided(),
                 "pat.lbeg:", dash::index(nview_cols_l).pattern().lbegin(),
                 "pat.lend:", dash::index(nview_cols_l).pattern().lend(),
                 "distance:", dash::distance(nview_cols_l.begin(),
                                             nview_cols_l.end()));
  dash::test::print_nview("cols_local_v", nview_cols_l);

  mat.barrier();

  auto nview_rows_l = dash::sub<0>(2,4, dash::local(dash::sub<0>(0,6, mat)));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "rows(local(mat)) ->",
                 dash::typestr(nview_rows_l));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "rows(local(mat)) ->",
                 "it:",       dash::typestr(nview_rows_l.begin()));
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "rows(local(mat)) ->",
                 "offsets:",  nview_rows_l.offsets(),
                 "extents:",  nview_rows_l.extents(),
                 "size:",     nview_rows_l.size());
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "index(rows(local(mat))) ->",
                 "strided:",  dash::index(nview_rows_l).is_strided(),
                 "pat.lbeg:", dash::index(nview_rows_l).pattern().lbegin(),
                 "pat.lend:", dash::index(nview_rows_l).pattern().lend(),
                 "distance:", dash::distance(nview_rows_l.begin(),
                                             nview_rows_l.end()));
  dash::test::print_nview("rows_local_v", nview_rows_l);

  return;

  EXPECT_EQ_U(mat.local_size(), dash::distance(nview_local.begin(),
                                               nview_local.end()));
  EXPECT_EQ_U(mat.local_size(), nview_local.size());
  EXPECT_EQ_U(mat.local_size(), dash::index(nview_local).size());

  EXPECT_EQ_U(mat.extent(0), nview_local.extent<0>());
  EXPECT_EQ_U(block_cols,    nview_local.extent<1>());
}

TEST_F(NViewTest, MatrixBlocked1DimBlocks)
{
  auto nunits = dash::size();

  int block_rows = 3;
  int block_cols = 2;

  if (nunits < 2) { block_cols = 8; }

  int nrows = nunits * block_rows;
  int ncols = nunits * block_cols;

  // columns distributed in blocks of same size:
  //
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //
  dash::Matrix<double, 2> mat_cb(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::NONE,
        dash::TILE(block_cols)),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat_cb);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "Matrix mat_cb initialized");

  if (dash::myid() == 0) {
    auto && v_mat_cb  = dash::sub<0>(0, mat_cb.extent(0), mat_cb);
    auto && cb_blocks = dash::blocks(v_mat_cb);
    EXPECT_EQ_U(nunits, cb_blocks.size());

    int bi = 0;
    for (auto block : cb_blocks) {
      DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                     "column block", bi, ":", range_str(block));
      bi++;
    }
  }

  // rows distributed in blocks of same size:
  //
  //  0 0 0 0 0 0 0 ...
  //  0 0 0 0 0 0 0 ...
  //  -----------------
  //  1 1 1 1 1 1 1 ...
  //  1 1 1 1 1 1 1 ...
  //
  dash::Matrix<double, 2> mat_rb(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::TILE(block_rows),
        dash::NONE),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat_rb);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                 "Matrix mat_rb initialized");

  if (dash::myid() == 0) {
    auto v_mat_rb  = dash::sub<0>(0, mat_rb.extent(0), mat_rb);
    auto rb_blocks = dash::blocks(v_mat_rb);
    EXPECT_EQ_U(nunits, rb_blocks.size());

    int bi = 0;
    for (auto block : rb_blocks) {
      DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSingle",
                     "row block", bi, ":", range_str(block));
      bi++;
    }
  }
}

TEST_F(NViewTest, MatrixBlocked1DimChained)
{
  auto nunits = dash::size();

  int block_rows = 3;
  int block_cols = 4;

  if (nunits < 2) { block_cols = 8; }

  int nrows = 2      * block_rows;
  int ncols = nunits * block_cols;

  // columns distributed in blocks of same size:
  //
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //
  dash::Matrix<double, 2> mat(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::NONE,
        dash::TILE(block_cols)),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained",
                 "Matrix initialized");

  // select first 2 matrix rows:
  auto nview_total  = dash::sub<0>(0, mat.extent(0), mat);
  auto nview_local  = dash::local(nview_total);

  if (dash::myid() == 0) {
    dash::test::print_nview("matrix.view",  nview_total);
  }
  mat.barrier();

  dash::test::print_nview("nview_local", nview_local);
  mat.barrier();

  auto nview_rows_g = dash::sub<0>(1, 3, mat);
  auto nview_cols_g = dash::sub<1>(2, 7, mat);

  auto nview_cr_s_g = dash::sub<1>(2, 7, dash::sub<0>(1, 3, mat));
  auto nview_rc_s_g = dash::sub<0>(1, 3, dash::sub<1>(2, 7, mat));

  EXPECT_EQ_U(2,             nview_rows_g.extent<0>());
  EXPECT_EQ_U(mat.extent(1), nview_rows_g.extent<1>());

  EXPECT_EQ_U(nview_rc_s_g.extents(), nview_cr_s_g.extents());
  EXPECT_EQ_U(nview_rc_s_g.offsets(), nview_cr_s_g.offsets());

  if (dash::myid() == 0) {
    dash::test::print_nview("nview_rows_g", nview_rows_g);
    dash::test::print_nview("nview_cols_g", nview_cols_g);

    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained",
                   "sub<1>(2,7, sub<0>(1,3, mat) ->",
                   "offsets:", nview_cr_s_g.offsets(),
                   "extents:", nview_cr_s_g.extents(),
                   "size:",    nview_cr_s_g.size());
    dash::test::print_nview("nview_cr_s_g", nview_cr_s_g);

    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained",
                   "sub<0>(1,3, sub<1>(2,7, mat) ->",
                   "offsets:", nview_rc_s_g.offsets(),
                   "extents:", nview_rc_s_g.extents(),
                   "size:",    nview_rc_s_g.size());
    dash::test::print_nview("nview_rc_s_g", nview_rc_s_g);

    auto exp_nview_cr_s_g = dash::test::region_values(
                              mat, {{ 1,2 }, { 2,5 }} );
    EXPECT_TRUE_U(
      dash::test::expect_range_values_equal<double>(
        exp_nview_cr_s_g, nview_cr_s_g));

    auto exp_nview_rc_s_g = dash::test::region_values(
                              mat, {{ 1,2 }, { 2,5 }} );
    EXPECT_TRUE_U(
      dash::test::expect_range_values_equal<double>(
        exp_nview_rc_s_g, nview_rc_s_g));
  }
  mat.barrier();

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained", "== nview_rows_l");
  auto nview_rows_l = dash::local(nview_rows_g);
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained",
                 "local(sub<0>(1,3, mat)):",
                 dash::typestr(nview_rows_l),
                 "extents:", nview_rows_l.extents(),
                 "offsets:", nview_rows_l.offsets());

// EXPECT_EQ_U(2,          nview_rows_l.extent<0>());
// EXPECT_EQ_U(block_cols, nview_rows_l.extent<1>());

// dash::test::print_nview("nview_rows_l", nview_rows_l);

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained", "== nview_cols_l");
  auto nview_cols_l = dash::local(nview_cols_g);
  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimChained",
                 "local(sub<1>(2,7, mat)):",
                 "extents:", nview_cols_l.extents(),
                 "offsets:", nview_cols_l.offsets());

// dash::test::print_nview("nview_cols_l", nview_cols_l);
}

TEST_F(NViewTest, MatrixBlocked1DimSub)
{
  auto nunits = dash::size();

  int block_rows = 4;
  int block_cols = 3;

  int nrows = nunits * block_rows;
  int ncols = nunits * block_cols;

  // columns distributed in blocks of same size:
  //
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //  0 0 0 | 1 1 1 | 2 2 2 | ...
  //
  dash::Matrix<double, 2> mat(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::NONE,
        dash::TILE(block_cols)),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat);

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", mat.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     mat.pattern().local_extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     mat.pattern().local_size());

  // Initial plausibility check: equality of iterator on n-dim view and
  // matrix view proxy iterator:
  auto view_expr_it    = dash::sub<0>(1,2, mat)
                           .begin() + 2;
  auto mat_ref_glob_it = (mat.row(1).begin() + 2);

  using globiter_t     = decltype(mat.begin());

  EXPECT_EQ_U(view_expr_it.dart_gptr(),
              mat_ref_glob_it.dart_gptr());

  if (dash::myid() == 0) {
    auto all_sub = dash::sub<0>(
                     0, mat.extents()[0],
                     mat);

    DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSub",
                   dash::typestr(all_sub));

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", all_sub.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", all_sub.extent(0));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", all_sub.extent(1));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", all_sub.size(0));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", all_sub.size(1));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                       index(all_sub).size());

    dash::test::print_nview("mat_view",  all_sub);
  }

  mat.barrier();

  // -- Sub-Section ----------------------------------
  //

  if (dash::myid() == 0) {
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", mat.extents());

    auto tmp        = dash::sub<1>(1, mat.extent(1) - 1,
                        mat);
    auto nview_sub  = dash::sub<0>(1, mat.extent(0) - 1,
                        tmp);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.extent(0));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.extent(1));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.size(0));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", nview_sub.size(1));
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                       index(nview_sub).size());

    dash::test::print_nview("nview_sub", nview_sub);

    auto nview_rows = nview_sub.extent<0>();
    auto nview_cols = nview_sub.extent<1>();

    EXPECT_EQ_U(nview_rows, nview_sub.extent(0));
    EXPECT_EQ_U(nview_rows, mat.extent(0) - 2);
    EXPECT_EQ_U(nview_cols, nview_sub.extent(1));
    EXPECT_EQ_U(nview_cols, mat.extent(1) - 2);

    auto exp_nview_sub = dash::test::region_values(
                              mat,
                              { { 1,1 },
                                { mat.extent(0) - 2, mat.extent(1) - 2 } });
    EXPECT_TRUE_U(
      dash::test::expect_range_values_equal<double>(
        exp_nview_sub, nview_sub));
  }

  // -- Local View -----------------------------------
  //

  auto lsub_view = dash::local(
                    dash::sub<0>(
                      0, mat.extents()[0],
                      mat));

  EXPECT_EQ_U(2, decltype(lsub_view)::rank::value);
  EXPECT_EQ_U(2, lsub_view.ndim());

  int  lrows    = lsub_view.extent<0>();
  int  lcols    = lsub_view.extent<1>();

  DASH_LOG_DEBUG("NViewTest.MatrixBlocked1DimSub",
                 dash::typestr(lsub_view));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.extent(0));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.extent(1));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.size(0));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.size(1));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub", lsub_view.size());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     index(lsub_view).size());

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     lsub_view.begin().pos());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     lsub_view.end().pos());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlocked1DimSub",
                     (lsub_view.end() - lsub_view.begin()));

  EXPECT_EQ_U(mat.local_size(), lrows * lcols);

  dash::test::print_nview("lsub_view",  lsub_view);
}

TEST_F(NViewTest, MatrixBlockCyclic1DimSub)
{
  auto nunits = dash::size();

  int block_rows = 4;
  int block_cols = 2;

  int nrows = block_rows;
  int ncols = nunits * block_cols * 2;

  // columns distributed in blocks of same size:
  //
  //  0 0 | 1 1 | 2 2 | 0 0  ....
  //  0 0 | 1 1 | 2 2 | 0 0  ....
  //  0 0 | 1 1 | 2 2 | 0 0  ....
  //
  dash::Matrix<double, 2> mat(
      dash::SizeSpec<2>(
        nrows,
        ncols),
      dash::DistributionSpec<2>(
        dash::NONE,
        dash::TILE(block_cols)),
      dash::Team::All(),
      dash::TeamSpec<2>(
        1,
        nunits));

  dash::test::initialize_matrix(mat);

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", mat.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                     mat.pattern().local_extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                     mat.pattern().local_size());

  if (dash::myid() == 0) {
    auto all_sub = dash::sub<0>(
                     0, mat.extents()[0],
                     mat);

    DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic1DSub",
                   dash::typestr(all_sub));

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", all_sub.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", all_sub.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       index(all_sub).size());

    dash::test::print_nview("mat_view",  all_sub);

    auto nview_rows  = dash::sub<0>(1, mat.extent(0) - 1, mat);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       nview_rows.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       nview_rows.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       index(nview_rows).size());

    dash::test::print_nview("nview_rows", nview_rows);

    auto nview_cols  = dash::sub<1>(1, mat.extent(1) - 1, mat);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       nview_cols.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       nview_cols.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                       index(nview_cols).size());

    dash::test::print_nview("nview_cols", nview_cols);

    auto nview_blocks  = dash::blocks(mat);

    int bi = 0;
    for (const auto & block : nview_blocks) {
      DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic1DSingle",
                     "block", bi, ":", "extents:", block.extents(),
                     range_str(block));
      bi++;
    }
  }
  mat.barrier();

  auto mat_loc  = dash::local(dash::sub<0>(0, mat.extent(0), mat));

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", mat_loc.offsets());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", mat_loc.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                     index(mat_loc).size());

  dash::test::print_nview("mat_loc", mat_loc);

  mat.barrier();

  auto loc_rows  = dash::local(dash::sub<0>(1, mat.extent(0) - 1, mat));

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", loc_rows.offsets());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", loc_rows.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                     index(loc_rows).size());

  //dash::test::print_nview("loc_rows", loc_rows);

  mat.barrier();

  auto loc_cols  = local(dash::sub<1>(1, mat.extent(1) - 1, mat));

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", loc_cols.offsets());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub", loc_cols.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic1DSub",
                     index(loc_cols).size());

  //dash::test::print_nview("loc_cols", loc_cols);
}

TEST_F(NViewTest, MatrixBlockCyclic2DimSub)
{
  auto nunits = dash::size();

  int block_rows = 3;
  int block_cols = 2;

  int nrows = nunits * block_rows;
  int ncols = nunits * block_cols * 2 - block_cols;

  if (nunits % 2 == 0 && nunits > 2) {
    nrows /= 2;
  }

  auto team_spec = dash::TeamSpec<2>(
                     nunits,
                     1);
  team_spec.balance_extents();

  auto pattern = dash::TilePattern<2>(
                   dash::SizeSpec<2>(
                     nrows,
                     ncols),
                   dash::DistributionSpec<2>(
                     dash::TILE(block_rows),
                     dash::TILE(block_cols)),
                   team_spec);

  using pattern_t = decltype(pattern);
  using index_t   = typename pattern_t::index_type;

  // columns distributed in blocks of same size:
  //
  //  0 0 | 1 1 | 2 2 | 0 0 ...
  //  0 0 | 1 1 | 2 2 | 0 0 ...
  //  ----+-----+-----+----
  //  1 1 | 2 2 | 0 0 | 1 1 ...
  //  1 1 | 2 2 | 0 0 | 1 1 ...
  //  ...   ...   ...   ...
  //
  dash::Matrix<double, 2, index_t, pattern_t> mat(pattern);

  dash::test::initialize_matrix(mat);

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", mat.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().local_extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().local_size());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().blockspec());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().blockspec().rank());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().blocksize(0));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().blocksize(1));
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().local_blockspec());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().local_blockspec().rank());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                     mat.pattern().teamspec());

  if (dash::myid() == 0) {
    auto all_sub = dash::sub<0>(
                     0, mat.extents()[0],
                     mat);

    DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic2DSub",
                   dash::typestr(all_sub));

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", all_sub.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", all_sub.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       index(all_sub).size());

    dash::test::print_nview("mat_global",  all_sub);
  }
  mat.barrier();

  if (dash::myid() == 0) {
    auto nview_rows  = dash::sub<0>(1, mat.extent(0) - 1, mat);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_rows.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_rows.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       index(nview_rows).size());

    dash::test::print_nview("nview_rows", nview_rows);

    auto nview_cols  = dash::sub<1>(1, mat.extent(1) - 1, mat);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_cols.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_cols.extents());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       index(nview_cols).size());

    dash::test::print_nview("nview_cols", nview_cols);

    auto nview_blocks  = dash::blocks(mat);

    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_blocks.size());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_blocks.offsets());
    DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub",
                       nview_blocks.extents());

    int bi = 0;
    for (const auto & block : nview_blocks) {
      DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic2DSub",
                     "block", bi, ":",
                     "offsets:", block.offsets(),
                     "extents:", block.extents());

      const auto & block_idx = dash::index(block);
      const auto & pat_block = mat.pattern().block(bi);

      DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic2DSub",
                     "blocks(mat)[b]:");
      DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic2DSub",
                     dash::test::nview_str(block));
      DASH_LOG_DEBUG("NViewTest.MatrixBlockCyclic2DSub",
                     "pattern.block(b):",
                     pat_block);

      EXPECT_EQ(pat_block.size(),    block.size());
      EXPECT_EQ(pat_block.offsets(), block.offsets());
      EXPECT_EQ(pat_block.extents(), block.extents());

      for (int bphase = 0; bphase < pat_block.size(); ++bphase) {
        auto pat_g_index  = dash::CartesianIndexSpace<2>(
                              mat.pattern().extents()
                            ).at(
                              // in-block coords
                              dash::CartesianIndexSpace<2>(
                                pat_block.extents()
                              ).coords(bphase),
                              // block viewspec
                              pat_block);
        EXPECT_EQ(pat_g_index, block_idx[bphase]);
      }
      bi++;
    }
  }
  mat.barrier();

  // TODO: multi-dimensional strided local views not supported yet,
  //       for example when using dash::SeqTilePattern in this test.

  auto mat_local = dash::local(
                     dash::sub<0>(
                       0, mat.extents()[0],
                       mat));

  typedef typename dash::pattern_traits<decltype(mat.pattern())>::mapping
                   pat_mapping_traits;
  bool pat_traits_shifted = pat_mapping_traits::shifted ||
                            pat_mapping_traits::diagonal;

  EXPECT_EQ_U(pat_traits_shifted, index(mat_local).is_shifted());

  EXPECT_TRUE_U(index(mat_local).is_strided() || dash::size() < 2);
  EXPECT_TRUE_U(index(mat_local).is_sub() || dash::size() < 2);
  EXPECT_FALSE_U(index(dash::domain(mat_local)).is_sub());

  EXPECT_EQ_U(mat.pattern().local_size(),    mat_local.size());
  EXPECT_EQ_U(mat.pattern().local_extents(), mat_local.extents());

  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", mat_local.offsets());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", mat_local.extents());
  DASH_LOG_DEBUG_VAR("NViewTest.MatrixBlockCyclic2DSub", mat_local.size());

  dash::test::print_nview("mat_local",  mat_local);
}


TEST_F(NViewTest, Matrix2DTiledLocalBlocks)
{
    dart_unit_t myid = dash::myid();
    size_t num_units = dash::Team::All().size();

    dash::TeamSpec<2> teamspec_2d(num_units, 1);
    teamspec_2d.balance_extents();

    //Number of blocks per dimension
    size_t size_factor = 3;
    size_t tile_size  = 3;
    size_t rows = tile_size * teamspec_2d.num_units(0) * size_factor;
    size_t cols = tile_size * teamspec_2d.num_units(1) * size_factor;
    size_t matrix_size = rows * cols;

    if (matrix_size <= 1024 && 0 == myid) {
        std::cout << "Matrix size: " << rows
                  << " x " << cols
                  << " == " << matrix_size
                  << std::endl;
    }

    dash::Matrix<double, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(tile_size),
                           dash::TILE(tile_size)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    std::fill(matrix.lbegin(), matrix.lend(), (double)myid);

    dash::barrier();

    auto pattern = matrix.pattern();
    using pattern_t = decltype(pattern);
    using index_t = pattern_t::index_type;
    using viewspec_t = pattern_t::viewspec_type;


  if (dash::myid() == 0) {
    dash::test::print_pattern_mapping(
      "matrix.pattern.unit_at", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> dart_unit_t {
          return _pattern.unit_at(
                   std::array<index_t, 2> {{ _x, _y }});
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.global_index", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _x * _pattern.extent(1) + _y;
      });
    dash::test::print_pattern_mapping(
      "matrix.pattern.local_index", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> index_t {
          return _pattern.local_index(
                   std::array<index_t, 2> {{ _x, _y }}).index;
      });


  }

  auto nlblocks = pattern.local_blockspec().size();

  std::vector<viewspec_t> local_blocks;

  for (auto b = 0; b < nlblocks; ++b) {
    auto block = pattern.local_block(b);
    local_blocks.push_back(block);
    DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
                   "matrix local_blockspec",
                   "extents:", block.extents(),
                   "offsets:", block.offsets(),
                   "size:",    block.size());
  }

  auto const last_block = local_blocks[local_blocks.size() - 1];
  auto const gend_row = (last_block.offset(0) + last_block.extent(0) - 1);
  auto const gend_col =  (last_block.offset(1) + last_block.extent(1));

  auto const gbegin_row = local_blocks[0].offset(0);
  auto const gbegin_col =  local_blocks[0].offset(1);

  auto loc = dash::local(matrix);

  DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
      "first_block gbegin", gbegin_row * cols + gbegin_col);
  DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
      "last_block gend", gend_row * cols + gend_col);

  DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
                 "local block view",
                 "extents:", loc.extents(),
                 "offsets:", loc.offsets(),
                 "size:",    loc.size());
  DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
                 "local block view domain extents:",
                 dash::domain(loc).extents());
  DASH_LOG_DEBUG("NViewTest.Matrix2DTiledLocalBlocks",
                 "begin.pos:",  loc.begin().pos(),
                 "end.pos:",    loc.end().pos(),
                 "begin.gpos:", loc.begin().gpos(),
                 "end.gpos:",   loc.end().gpos());


  DASH_ASSERT(loc.begin().gpos() == gbegin_row * cols + gbegin_col);
  DASH_ASSERT(loc.end().gpos() == gend_row * cols + gend_col);
}

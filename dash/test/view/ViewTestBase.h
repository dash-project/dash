#ifndef DASH__TEST__VIEW__VIEW_TEST_BASE_H__INCLUDED
#define DASH__TEST__VIEW__VIEW_TEST_BASE_H__INCLUDED

#include "../TestBase.h"

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

template <typename RangeT>
bool is_contiguous_ix(const RangeT & rng) {
  if (!rng || rng.size() == 1) { return true; }
  auto ix_prev = rng.first();
  for (const auto & ix : rng) {
    if ((ix != ix_prev+1) &&
        !(ix == ix_prev && ix == rng.first())) { return false; }
    ix_prev = ix;
  }
  return true;
}

using dash::sub;
using dash::block;
using dash::blocks;
using dash::local;
using dash::index;
using dash::expand;
using dash::shift;

class ViewTestBase : public dash::test::TestBase {
protected:
};

} // namespace test
} // namespace dash

#endif // DASH__TEST__VIEW__VIEW_TEST_BASE_H__INCLUDED


#include "HaloTest.h"

#include <dash/Matrix.h>
#include <dash/Algorithm.h>
#include <dash/halo/HaloMatrixWrapper.h>

#include <iostream>

using namespace dash;

using namespace dash::halo;

TEST_F(HaloTest, GlobalBoundarySpec)
{
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;

  GlobBoundSpec_t bound_spec_1;
  EXPECT_EQ(bound_spec_1[0], BoundaryProp::NONE);
  EXPECT_EQ(bound_spec_1[1], BoundaryProp::NONE);
  EXPECT_EQ(bound_spec_1[2], BoundaryProp::NONE);

  GlobBoundSpec_t bound_spec_2(BoundaryProp::CYCLIC, BoundaryProp::NONE, BoundaryProp::CUSTOM);
  EXPECT_EQ(bound_spec_2[0], BoundaryProp::CYCLIC);
  EXPECT_EQ(bound_spec_2[1], BoundaryProp::NONE);
  EXPECT_EQ(bound_spec_2[2], BoundaryProp::CUSTOM);
}

TEST_F(HaloTest, CoordsToIndex)
{
  using RCoords_t = RegionCoords<3>;

  RCoords_t coords({1,0,1});
  EXPECT_EQ(10, coords.index());
  RCoords_t coords2({2,2,2});
  EXPECT_EQ(26, coords2.index());
}

TEST_F(HaloTest, IndexToCoords)
{
  using RCoords_t = RegionCoords<3>;

  EXPECT_EQ(RCoords_t({1,0,1}), RCoords_t(10));
  EXPECT_EQ(RCoords_t({2,2,2}), RCoords_t(26));
}

TEST_F(HaloTest, HaloSpec)
{
  using HaloRegSpec_t = RegionSpec<3>;
  using HaloSpec_t    = HaloSpec<3>;
  using RCoords_t     = RegionCoords<3>;

  HaloSpec_t specs(
      HaloRegSpec_t({0,0,0}, 3),
      HaloRegSpec_t({0,1,1}, 2),
      HaloRegSpec_t({2,1,0}, 2),
      HaloRegSpec_t({2,2,2}, 3));

  EXPECT_EQ(specs.spec(0).coords(), RCoords_t({0,0,0}));
  EXPECT_EQ(specs.spec(4).coords(), RCoords_t({0,1,1}));
  EXPECT_EQ(specs.spec(21).coords(), RCoords_t({2,1,0}));
  EXPECT_EQ(specs.spec(26).coords(), RCoords_t({2,2,2}));
  EXPECT_EQ((uint32_t)specs.extent(0), 3);
  EXPECT_EQ((uint32_t)specs.extent(4), 2);
  EXPECT_EQ((uint32_t)specs.extent(21), 2);
  EXPECT_EQ((uint32_t)specs.extent(26), 3);

}

TEST_F(HaloTest, HaloSpecStencils)
{
  using HaloRegSpec_t = RegionSpec<3>;
  using HaloSpec_t    = HaloSpec<3>;
  using RCoords_t     = RegionCoords<3>;
  using StencilP_t    = StencilPoint<3>;

  {
    using StencilSpec_t = StencilSpec<StencilP_t, 6>;
    StencilSpec_t stencil_spec(
       StencilP_t(0.2, -1,  0,  0), StencilP_t(0.4, 1, 0, 0),
       StencilP_t( 0, -1,  0), StencilP_t(0, 2, 0),
       StencilP_t( 0,  0, -1), StencilP_t(0, 0, 3));
    HaloSpec_t halo_spec(stencil_spec);

    EXPECT_EQ(stencil_spec[0].coefficient(), 0.2);
    EXPECT_EQ(stencil_spec[1].coefficient(), 0.4);
    EXPECT_EQ(stencil_spec[2].coefficient(), 1.0);

    EXPECT_EQ(halo_spec.spec(4).coords(), RCoords_t({0,1,1}));
    EXPECT_EQ(halo_spec.spec(22).coords(), RCoords_t({2,1,1}));
    EXPECT_EQ(halo_spec.spec(10).coords(), RCoords_t({1,0,1}));
    EXPECT_EQ(halo_spec.spec(16).coords(), RCoords_t({1,2,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoords_t({1,1,0}));
    EXPECT_EQ(halo_spec.spec(14).coords(), RCoords_t({1,1,2}));
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(14), 3);
  }
  {
    using StencilSpec_t = StencilSpec<StencilP_t, 1>;

    StencilSpec_t stencil_spec( StencilP_t(0, 2, 0));
    HaloSpec_t halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(16).coords(), RCoords_t({1,2,1}));
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 2);
    for(auto i = 0; i < RCoords_t::MaxIndex; ++i) {
      if(i != 16)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    using StencilSpec_t = StencilSpec<StencilP_t, 1>;

    StencilSpec_t stencil_spec(StencilP_t(-2, 0, -1));
    HaloSpec_t halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(3).coords(), RCoords_t({0,1,0}));
    EXPECT_EQ(halo_spec.spec(4).coords(), RCoords_t({0,1,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoords_t({1,1,0}));
    EXPECT_EQ((uint32_t)halo_spec.extent(3), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    for(auto i = 0; i < RCoords_t::MaxIndex; ++i) {
      if(i != 3 && i != 4 && i != 12)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    using StencilSpec_t = StencilSpec<StencilP_t, 1>;

    StencilSpec_t stencil_spec(StencilP_t(-3, -2, -1));
    HaloSpec_t halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(0).coords(), RCoords_t({0,0,0}));
    EXPECT_EQ(halo_spec.spec(1).coords(), RCoords_t({0,0,1}));
    EXPECT_EQ(halo_spec.spec(3).coords(), RCoords_t({0,1,0}));
    EXPECT_EQ(halo_spec.spec(4).coords(), RCoords_t({0,1,1}));
    EXPECT_EQ(halo_spec.spec(9).coords(), RCoords_t({1,0,0}));
    EXPECT_EQ(halo_spec.spec(10).coords(), RCoords_t({1,0,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoords_t({1,1,0}));
    EXPECT_EQ((uint32_t)halo_spec.extent(0), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(1), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(3), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(9), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    for(auto i = 0; i < RCoords_t::MaxIndex; ++i) {
      if(i != 0 && i != 1 && i != 3 && i != 4 && i != 9 && i != 10 && i!= 12)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    using StencilSpec_t = StencilSpec<StencilP_t, 1>;

    StencilSpec_t stencil_spec(StencilP_t(3, 3, 3));
    HaloSpec_t halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(14).coords(), RCoords_t({1,1,2}));
    EXPECT_EQ(halo_spec.spec(16).coords(), RCoords_t({1,2,1}));
    EXPECT_EQ(halo_spec.spec(17).coords(), RCoords_t({1,2,2}));
    EXPECT_EQ(halo_spec.spec(22).coords(), RCoords_t({2,1,1}));
    EXPECT_EQ(halo_spec.spec(23).coords(), RCoords_t({2,1,2}));
    EXPECT_EQ(halo_spec.spec(25).coords(), RCoords_t({2,2,1}));
    EXPECT_EQ(halo_spec.spec(26).coords(), RCoords_t({2,2,2}));
    EXPECT_EQ((uint32_t)halo_spec.extent(14), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(17), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(22), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(23), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(25), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(26), 3);
    for(auto i = 0; i < RCoords_t::MaxIndex; ++i) {
      if(i != 14 && i != 16 && i != 17 && i != 22 && i != 23 && i != 25 && i!= 26)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
}

TEST_F(HaloTest, HaloMatrixWrapperNonCyclic2D)
{
  using Pattern_t  = dash::Pattern<2>;
  using index_type = typename Pattern_t::index_type;
  using Matrix_t   = dash::Matrix<long, 2, index_type, Pattern_t>;
  using DistSpec_t = dash::DistributionSpec<2>;
  using TeamSpec_t = dash::TeamSpec<2>;
  using SizeSpec_t = dash::SizeSpec<2>;

  using GlobBoundSpec_t = GlobalBoundarySpec<2>;
  using StencilP_t      = StencilPoint<2>;
  using StencilSpec_t   = StencilSpec<StencilP_t, 8>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);
  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  dash::Team::All().barrier();

  long sum_check = 0;
  if(myid == 0) {
    long** matrix_check = new long*[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long[ext_per_dim];
      for(auto j = 0; j < ext_per_dim; ++j) {
        if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff) {
          matrix_check[i][j] = 5;
          continue;
        }

        matrix_check[i][j] = 1;
      }
    }

    for(index_type i = 1; i < ext_per_dim - 1; ++i) {
      for(index_type j = 1; j < ext_per_dim - 1; ++j) {
        sum_check += matrix_check[i-1][j-1] + matrix_check[i-1][j]   + matrix_check[i-1][j+1] +
                    matrix_check[i][j-1]   + matrix_check[i][j]     + matrix_check[i][j+1] +
                    matrix_check[i+1][j-1] + matrix_check[i+1][j] + matrix_check[i+1][j+1];
      }
    }

    for(auto i = 0; i < ext_per_dim; ++i)
      delete[] matrix_check[i];
    delete[] matrix_check;
  }

  for(auto i = 0; i < ext_per_dim; ++i) {
    for(auto j = 0; j < ext_per_dim; ++j) {
      if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff) {
        auto tmp = matrix_halo[i][j];
        if(tmp.is_local())
          tmp = 5;
      }
    }
  }
  matrix_halo.barrier();

  GlobBoundSpec_t bound_spec;
  StencilSpec_t stencil_spec(
      StencilP_t(-1,-1), StencilP_t(-1, 0), StencilP_t(-1, 1),
      StencilP_t( 0,-1),                    StencilP_t( 0, 1),
      StencilP_t( 1,-1), StencilP_t( 1, 0), StencilP_t( 1, 1));

  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, stencil_spec);

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);
  auto* sum_local = sum_halo.lbegin();
  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);

  auto it_iend = stencil_op.inner.end();
  for(auto it = stencil_op.inner.begin(); it != it_iend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(i);

    *sum_local += *it;
  }

  halo_wrapper.update();
  auto it_bend = stencil_op.boundary.end();
  for(auto it = stencil_op.boundary.begin(); it != it_bend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(i);

    *sum_local += *it;
  }

  sum_halo.barrier();

  unsigned long sum_halo_total = 0;
  if(dash::myid() == 0) {
    for(const auto& elem : sum_halo)
      sum_halo_total += elem;

    EXPECT_EQ(sum_check, sum_halo_total);
  }

  dash::Team::All().barrier();
}

template<typename T>
long calc_sum_check(long*** matrix, T begin, T end) {

  unsigned long sum = 0;
  for(auto i = begin; i < end; ++i) {
    for(auto j = begin; j < end; ++j) {
      for(auto k = begin; k < end; ++k) {
        sum +=
          matrix[i-1][j-1][k-1] + matrix[i-1][j-1][k] + matrix[i-1][j-1][k+1] +
          matrix[i-1][j][k-1]   + matrix[i-1][j][k]   + matrix[i-1][j][k+1] +
          matrix[i-1][j+1][k-1] + matrix[i-1][j+1][k] + matrix[i-1][j+1][k+1] +
          matrix[i][j-1][k-1]   + matrix[i][j-1][k]   + matrix[i][j-1][k+1] +
          matrix[i][j][k-1]     + matrix[i][j][k]     + matrix[i][j][k+1] +
          matrix[i][j+1][k-1]   + matrix[i][j+1][k]   + matrix[i][j+1][k+1] +
          matrix[i+1][j-1][k-1] + matrix[i+1][j-1][k] + matrix[i+1][j-1][k+1] +
          matrix[i+1][j][k-1]   + matrix[i+1][j][k]   + matrix[i+1][j][k+1] +
          matrix[i+1][j+1][k-1] + matrix[i+1][j+1][k] + matrix[i+1][j+1][k+1];
      }
    }
  }

  return sum;
}

template<typename HaloWrapperT, typename StencilOpT>
unsigned long calc_sum_halo(HaloWrapperT& halo_wrapper, StencilOpT stencil_op, bool region_wise = false) {
  auto& stencil_spec = stencil_op.stencil_spec();
  auto num_stencil_points = stencil_spec.num_stencil_points();

  halo_wrapper.update_async();

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);

  auto* sum_local = sum_halo.lbegin();
  auto it_iend = stencil_op.inner.end();
  for(auto it = stencil_op.inner.begin(); it != it_iend; ++it) {
    for(auto i = 0; i < num_stencil_points; ++i)
      *sum_local += it.value_at(i);

    *sum_local += *it;
  }

  halo_wrapper.wait();

  if(region_wise) {
    for( auto d = 0; d < 3; ++d) {
      auto it_bnd = stencil_op.boundary.iterator_at(d, RegionPos::PRE);
      for(auto it = it_bnd.first; it != it_bnd.second; ++it) {
        for(auto i = 0; i < num_stencil_points; ++i)
          *sum_local += it.value_at(i);

        *sum_local += *it;
      }
      auto it_bnd_2 = stencil_op.boundary.iterator_at(d, RegionPos::POST);
      for(auto it = it_bnd_2.first; it != it_bnd_2.second; ++it) {
        for(auto i = 0; i < num_stencil_points; ++i)
          *sum_local += it.value_at(i);

        *sum_local += *it;
      }
    }
  } else {
    auto it_bend = stencil_op.boundary.end();
    for(auto it = stencil_op.boundary.begin(); it != it_bend; ++it) {
      for(auto i = 0; i < num_stencil_points; ++i)
        *sum_local += it.value_at(i);

      *sum_local += *it;
    }
  }
  sum_halo.barrier();

  unsigned long sum = 0;
  if(dash::myid() == 0) {
    for(const auto& elem : sum_halo)
      sum += elem;

  }

  return sum;
}

template<typename HaloWrapperT, typename StencilOpT>
unsigned long calc_sum_halo_via_stencil(HaloWrapperT& halo_wrapper, StencilOpT stencil_op) {
  auto& stencil_spec = stencil_op.stencil_spec();
  auto num_stencil_points = stencil_spec.num_stencil_points();

  halo_wrapper.update_async();

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);

  auto* sum_local = sum_halo.lbegin();
  auto it_iend = stencil_op.inner.end();
  for(auto it = stencil_op.inner.begin(); it != it_iend; ++it) {
    for(auto i = 0; i < num_stencil_points; ++i)
      *sum_local += it.value_at(stencil_spec[i]);

    *sum_local += *it;
  }

  halo_wrapper.wait();

  auto it_bend = stencil_op.boundary.end();
  for(auto it = stencil_op.boundary.begin(); it != it_bend; ++it) {
    for(auto i = 0; i < num_stencil_points; ++i)
      *sum_local += it.value_at(stencil_spec[i]);

    *sum_local += *it;
  }

  sum_halo.barrier();

  unsigned long sum = 0;
  if(dash::myid() == 0) {
    for(const auto& elem : sum_halo)
      sum += elem;

  }

  return sum;
}

template<typename MatrixT>
void HaloTest::init_matrix3D(MatrixT& matrix) {
  for(auto i = 0; i < ext_per_dim; ++i) {
    for(auto j = 0; j < ext_per_dim; ++j) {
      for(auto k = 0; k < ext_per_dim; ++k) {
        if(i == 0 || i == ext_per_dim - 1 || j == 0 || j == ext_per_dim - 1 ||
           k == 0 || k == ext_per_dim - 1) {
            auto tmp = matrix[i][j][k];
            if(tmp.is_local())
              tmp = 10;
          continue;
        }
        if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff &&
           k >= boundary_width && k < ext_diff) {
          auto tmp = matrix[i][j][k];
          if(tmp.is_local())
            tmp = 5;
          continue;
        }
      }
    }
  }
}

TEST_F(HaloTest, HaloMatrixWrapperNonCyclic3D)
{
  using Pattern_t = dash::Pattern<3>;
  using index_type = typename Pattern_t::index_type;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using DistSpec_t = dash::DistributionSpec<3>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;

  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;
  using StencilSpec_t = StencilSpec<StencilP_t, 26>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  unsigned long sum_check = 0;

  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long*[ext_per_dim];
      for(auto j = 0; j < ext_per_dim; ++j) {
        matrix_check[i][j] = new long[ext_per_dim];
        for(auto k = 0; k < ext_per_dim; ++k) {
          if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff &&
             k >= boundary_width && k < ext_diff) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }

    sum_check += calc_sum_check(matrix_check, 1l, ext_per_dim - 1);

    for(auto i = 0; i < ext_per_dim; ++i) {
      for(auto j = 0; j < ext_per_dim; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;
  }

  for(auto i = 0; i < ext_per_dim; ++i) {
    for(auto j = 0; j < ext_per_dim; ++j) {
      for(auto k = 0; k < ext_per_dim; ++k) {
        if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff &&
           k >= boundary_width && k < ext_diff) {
          auto tmp = matrix_halo[i][j][k];
          if(tmp.is_local())
            tmp = 5;
        }
      }
    }
  }
  matrix_halo.barrier();

  StencilSpec_t stencil_spec(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 0), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 0,-1), StencilP_t(-1, 0, 0), StencilP_t(-1, 0, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 0), StencilP_t(-1, 1, 1),
      StencilP_t( 0,-1,-1), StencilP_t( 0,-1, 0), StencilP_t( 0,-1, 1),
      StencilP_t( 0, 0,-1),                     StencilP_t( 0, 0, 1),
      StencilP_t( 0, 1,-1), StencilP_t( 0, 1, 0), StencilP_t( 0, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 0), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 0,-1), StencilP_t( 1, 0, 0), StencilP_t( 1, 0, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 0), StencilP_t( 1, 1, 1)
  );

  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, stencil_spec);
  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper, stencil_op);
  if(myid == 0) {
    // global outer boundary not included in both sums
    EXPECT_EQ(sum_check, sum_halo);
  }
  dash::Team::All().barrier();

}


TEST_F(HaloTest, HaloMatrixWrapperCyclic3D)
{
  using Pattern_t = dash::Pattern<3>;
  using PatternCol_t = dash::Pattern<3, dash::COL_MAJOR>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using MatrixCol_t = dash::Matrix<long, 3, index_type, PatternCol_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;
  using StencilSpec_t = StencilSpec<StencilP_t, 26>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());
  PatternCol_t pattern_col(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);
  MatrixCol_t matrix_halo_col(pattern_col);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);
  dash::fill(matrix_halo_col.begin(), matrix_halo_col.end(), 1);

  dash::Team::All().barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim_check];
    for(auto i = 0; i < ext_per_dim_check; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if(i == 0 || i == ext_per_dim_check - 1 || j == 0 || j == ext_per_dim_check - 1 ||
             k == 0 || k == ext_per_dim_check -1) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i == 1 || i == ext_per_dim_check - 2 || j == 1 || j == ext_per_dim_check - 2 ||
             k == 1 || k == ext_per_dim_check -2) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i > boundary_width  && i <= ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }

    sum_check += calc_sum_check(matrix_check, 1l, ext_per_dim_check - 1);

    for(auto i = 0; i < ext_per_dim_check; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;
  }

  init_matrix3D(matrix_halo);
  init_matrix3D(matrix_halo_col);

  dash::Team::All().barrier();

  StencilSpec_t stencil_spec(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 0), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 0,-1), StencilP_t(-1, 0, 0), StencilP_t(-1, 0, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 0), StencilP_t(-1, 1, 1),
      StencilP_t( 0,-1,-1), StencilP_t( 0,-1, 0), StencilP_t( 0,-1, 1),
      StencilP_t( 0, 0,-1),                     StencilP_t( 0, 0, 1),
      StencilP_t( 0, 1,-1), StencilP_t( 0, 1, 0), StencilP_t( 0, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 0), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 0,-1), StencilP_t( 1, 0, 0), StencilP_t( 1, 0, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 0), StencilP_t( 1, 1, 1)
  );
  GlobBoundSpec_t bound_spec(BoundaryProp::CYCLIC, BoundaryProp::CYCLIC, BoundaryProp::CYCLIC);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec);
  HaloMatrixWrapper<MatrixCol_t> halo_wrapper_col(matrix_halo_col, bound_spec, stencil_spec);


  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper, stencil_op);

  auto stencil_op_col = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo_col = calc_sum_halo(halo_wrapper_col, stencil_op_col);

  if(myid == 0) {
    EXPECT_EQ(sum_check, sum_halo);
    EXPECT_EQ(sum_check, sum_halo_col);
  }
  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperCustom3D)
{
  using Pattern_t = dash::Pattern<3>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;
  using StencilSpec_t = StencilSpec<StencilP_t, 26>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim_check];
    for(auto i = 0; i < ext_per_dim_check; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if(i == 0 || i == ext_per_dim_check - 1 || j == 0 || j == ext_per_dim_check - 1 ||
             k == 0 || k == ext_per_dim_check - 1) {
            matrix_check[i][j][k] = 20;
            continue;
          }
          if(i == 1 || i == ext_per_dim_check - 2 || j == 1 || j == ext_per_dim_check - 2 ||
             k == 1 || k == ext_per_dim_check -2) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i > boundary_width  && i <= ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }

    sum_check += calc_sum_check(matrix_check, 1l, ext_per_dim_check - 1);

    for(auto i = 0; i < ext_per_dim_check; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;
  }

  init_matrix3D(matrix_halo);

  matrix_halo.barrier();

  StencilSpec_t stencil_spec(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 0), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 0,-1), StencilP_t(-1, 0, 0), StencilP_t(-1, 0, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 0), StencilP_t(-1, 1, 1),
      StencilP_t( 0,-1,-1), StencilP_t( 0,-1, 0), StencilP_t( 0,-1, 1),
      StencilP_t( 0, 0,-1),                     StencilP_t( 0, 0, 1),
      StencilP_t( 0, 1,-1), StencilP_t( 0, 1, 0), StencilP_t( 0, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 0), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 0,-1), StencilP_t( 1, 0, 0), StencilP_t( 1, 0, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 0), StencilP_t( 1, 1, 1)
  );

  GlobBoundSpec_t bound_spec(BoundaryProp::CUSTOM, BoundaryProp::CUSTOM, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec);

  halo_wrapper.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper, stencil_op);

  if(myid == 0)
    EXPECT_EQ(sum_check, sum_halo);

  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperMix3D)
{
  using Pattern_t = dash::Pattern<3>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;
  using StencilSpec_t = StencilSpec<StencilP_t, 26>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if((j == 0 || j == ext_per_dim_check - 1) && k != 0 && k != ext_per_dim_check -1) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(k == 0 || k == ext_per_dim_check -1) {
            matrix_check[i][j][k] = 20;
            continue;
          }
          if(i == 0 || i == ext_per_dim - 1 || j == 1 || j == ext_per_dim_check - 2 ||
             k == 1 || k == ext_per_dim_check -2) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }

    for(auto i = 1; i < ext_per_dim - 1; ++i) {
      for(auto j = 1; j < ext_per_dim_check -1; ++j) {
        for(auto k = 1; k < ext_per_dim_check - 1; ++k) {
          sum_check +=
            matrix_check[i-1][j-1][k-1] + matrix_check[i-1][j-1][k] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-1][j][k-1]   + matrix_check[i-1][j][k]   + matrix_check[i-1][j][k+1] +
            matrix_check[i-1][j+1][k-1] + matrix_check[i-1][j+1][k] + matrix_check[i-1][j+1][k+1] +
            matrix_check[i][j-1][k-1]   + matrix_check[i][j-1][k]   + matrix_check[i][j-1][k+1] +
            matrix_check[i][j][k-1]     + matrix_check[i][j][k]     + matrix_check[i][j][k+1] +
            matrix_check[i][j+1][k-1]   + matrix_check[i][j+1][k]   + matrix_check[i][j+1][k+1] +
            matrix_check[i+1][j-1][k-1] + matrix_check[i+1][j-1][k] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+1][j][k-1]   + matrix_check[i+1][j][k]   + matrix_check[i+1][j][k+1] +
            matrix_check[i+1][j+1][k-1] + matrix_check[i+1][j+1][k] + matrix_check[i+1][j+1][k+1];
        }
      }
    }
    for(auto i = 0; i < ext_per_dim; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;
  }

  init_matrix3D(matrix_halo);

  matrix_halo.barrier();

  StencilSpec_t stencil_spec(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 0), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 0,-1), StencilP_t(-1, 0, 0), StencilP_t(-1, 0, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 0), StencilP_t(-1, 1, 1),
      StencilP_t( 0,-1,-1), StencilP_t( 0,-1, 0), StencilP_t( 0,-1, 1),
      StencilP_t( 0, 0,-1),                     StencilP_t( 0, 0, 1),
      StencilP_t( 0, 1,-1), StencilP_t( 0, 1, 0), StencilP_t( 0, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 0), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 0,-1), StencilP_t( 1, 0, 0), StencilP_t( 1, 0, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 0), StencilP_t( 1, 1, 1)
  );
  GlobBoundSpec_t bound_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec);

  halo_wrapper.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper, stencil_op);

  if(myid == 0)
    EXPECT_EQ(sum_check, sum_halo);

  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperBigMix3D)
{
  using Pattern_t = dash::Pattern<3>;
  using PatternCol_t = dash::Pattern<3, dash::COL_MAJOR>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using MatrixCol_t = dash::Matrix<long, 3, index_type, PatternCol_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;
  using StencilSpec_t = StencilSpec<StencilP_t, 26>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());
  PatternCol_t pattern_col(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);
  MatrixCol_t matrix_halo_col(pattern_col);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);
  dash::fill(matrix_halo_col.begin(), matrix_halo_col.end(), 1);

  dash::Team::All().barrier();

  auto ext_per_dim_check = ext_per_dim + 6;
  unsigned long sum_check = 0;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if((j == 2 || j == ext_per_dim_check - 3) && k > 2 && k < ext_per_dim_check -3) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if((j < 3 || j >= ext_per_dim_check - 3) && k > 3 && k < ext_per_dim_check - 4) {
            if(i == 0 || i == ext_per_dim - 1)
              matrix_check[i][j][k] = 10;
            else
              matrix_check[i][j][k] = 1;
            continue;
          }
          if(k < 3 || k >= ext_per_dim_check -3) {
            matrix_check[i][j][k] = 20;
            continue;
          }
          if(i == 0 || i == ext_per_dim - 1 || j == 3 || j == ext_per_dim_check - 4 ||
             k == 3 || k == ext_per_dim_check - 4) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j >= boundary_width + 3 && j < ext_diff + 3 &&
             k >= boundary_width + 3 && k < ext_diff + 3) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }

    for(auto i = 3; i < ext_per_dim - 3; ++i) {
      for(auto j = 3; j < ext_per_dim_check -3; ++j) {
        for(auto k = 3; k < ext_per_dim_check - 3; ++k) {
          sum_check +=
            matrix_check[i-3][j-3][k-3] + matrix_check[i-2][j-2][k-2] + matrix_check[i-1][j-1][k-1] +
            matrix_check[i-3][j-3][k+3] + matrix_check[i-2][j-2][k+2] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-3][j+3][k-3] + matrix_check[i-2][j+2][k-2] + matrix_check[i-1][j+1][k-1] +
            matrix_check[i-3][j][k]     + matrix_check[i-2][j][k]     + matrix_check[i-1][j][k] +
            matrix_check[i][j-2][k]     + matrix_check[i][j][k]       + matrix_check[i][j+2][k] +
            matrix_check[i+3][j][k]     + matrix_check[i+2][j][k]     + matrix_check[i+1][j][k] +
            matrix_check[i+3][j-3][k+3] + matrix_check[i+2][j-2][k+2] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+3][j+3][k-3] + matrix_check[i+2][j+2][k-2] + matrix_check[i+1][j+1][k-1] +
            matrix_check[i+3][j+3][k+3] + matrix_check[i+2][j+2][k+2] + matrix_check[i+1][j+1][k+1];
        }
      }
    }


    for(auto i = 0; i < ext_per_dim; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;

  }

  init_matrix3D(matrix_halo);
  init_matrix3D(matrix_halo_col);

  dash::Team::All().barrier();

  StencilSpec_t stencil_spec(
      StencilP_t(-3,-3,-3), StencilP_t(-2,-2,-2), StencilP_t(-1,-1,-1),
      StencilP_t(-3,-3, 3), StencilP_t(-2,-2, 2), StencilP_t(-1,-1, 1),
      StencilP_t(-3, 3,-3), StencilP_t(-2, 2,-2), StencilP_t(-1, 1,-1),
      StencilP_t(-3, 0, 0), StencilP_t(-2, 0, 0), StencilP_t(-1, 0, 0),
      StencilP_t( 0,-2, 0),                     StencilP_t( 0, 2, 0),
      StencilP_t( 3, 0, 0), StencilP_t( 2, 0, 0), StencilP_t( 1, 0, 0),
      StencilP_t( 3,-3, 3), StencilP_t( 2,-2, 2), StencilP_t( 1,-1, 1),
      StencilP_t( 3, 3,-3), StencilP_t( 2, 2,-2), StencilP_t( 1, 1,-1),
      StencilP_t( 3, 3, 3), StencilP_t( 2, 2, 2), StencilP_t( 1, 1, 1)
  );
  GlobBoundSpec_t bound_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec);
  HaloMatrixWrapper<MatrixCol_t> halo_wrapper_col(matrix_halo_col, bound_spec, stencil_spec);

  halo_wrapper.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op = halo_wrapper.stencil_operator(stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper, stencil_op);
  auto sum_halo_region = calc_sum_halo(halo_wrapper, stencil_op, true);
  auto sum_halo_via_stencil = calc_sum_halo_via_stencil(halo_wrapper, stencil_op);

  halo_wrapper_col.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op_col = halo_wrapper_col.stencil_operator(stencil_spec);
  auto sum_halo_col = calc_sum_halo(halo_wrapper_col, stencil_op_col);

  if(myid == 0) {
    EXPECT_EQ(sum_check, sum_halo);
    EXPECT_EQ(sum_check, sum_halo_region);
    EXPECT_EQ(sum_check, sum_halo_via_stencil);
    EXPECT_EQ(sum_check, sum_halo_col);
  }
  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperMultiStencil3D)
{
  using Pattern_t = dash::Pattern<3>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  unsigned long sum_check_spec_1 = 0;
  unsigned long sum_check_spec_2 = 0;
  unsigned long sum_check_spec_3 = 0;

  auto ext_per_dim_check = ext_per_dim + 2;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if((j == 0 || j == ext_per_dim_check - 1) && k != 0 && k != ext_per_dim_check -1) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(k == 0 || k == ext_per_dim_check -1) {
            matrix_check[i][j][k] = 20;
            continue;
          }
          if(i == 0 || i == ext_per_dim - 1 || j == 1 || j == ext_per_dim_check - 2 ||
             k == 1 || k == ext_per_dim_check -2) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }
    for(auto i = 1; i < ext_per_dim - 1; ++i) {
      for(auto j = 1; j < ext_per_dim_check -1; ++j) {
        for(auto k = 1; k < ext_per_dim_check - 1; ++k) {
          sum_check_spec_1 += matrix_check[i][j][k] +
            matrix_check[i-1][j][k] + matrix_check[i+1][j][k] +
            matrix_check[i][j-1][k] + matrix_check[i][j+1][k] +
            matrix_check[i][j][k-1] + matrix_check[i][j][k+1];
          sum_check_spec_2 +=
            matrix_check[i-1][j-1][k-1] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-1][j+1][k-1] + matrix_check[i-1][j+1][k+1] +
            matrix_check[i][j][k] +
            matrix_check[i+1][j-1][k-1] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+1][j+1][k-1] + matrix_check[i+1][j+1][k+1];
          sum_check_spec_3 +=
            matrix_check[i-1][j-1][k-1] + matrix_check[i-1][j-1][k] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-1][j][k-1]   + matrix_check[i-1][j][k]   + matrix_check[i-1][j][k+1] +
            matrix_check[i-1][j+1][k-1] + matrix_check[i-1][j+1][k] + matrix_check[i-1][j+1][k+1] +
            matrix_check[i][j-1][k-1]   + matrix_check[i][j-1][k]   + matrix_check[i][j-1][k+1] +
            matrix_check[i][j][k-1]     + matrix_check[i][j][k]     + matrix_check[i][j][k+1] +
            matrix_check[i][j+1][k-1]   + matrix_check[i][j+1][k]   + matrix_check[i][j+1][k+1] +
            matrix_check[i+1][j-1][k-1] + matrix_check[i+1][j-1][k] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+1][j][k-1]   + matrix_check[i+1][j][k]   + matrix_check[i+1][j][k+1] +
            matrix_check[i+1][j+1][k-1] + matrix_check[i+1][j+1][k] + matrix_check[i+1][j+1][k+1];
        }
      }
    }
    for(auto i = 0; i < ext_per_dim; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;
  }

  init_matrix3D(matrix_halo);

  matrix_halo.barrier();

  StencilSpec<StencilP_t, 6> stencil_spec_1(
      StencilP_t(-1, 0, 0), StencilP_t( 1, 0, 0),
      StencilP_t( 0,-1, 0), StencilP_t( 0, 1, 0),
      StencilP_t( 0, 0,-1), StencilP_t( 0, 0, 1)
  );
  StencilSpec<StencilP_t, 8> stencil_spec_2(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 1)
  );
  StencilSpec<StencilP_t, 26> stencil_spec_3(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 0), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 0,-1), StencilP_t(-1, 0, 0), StencilP_t(-1, 0, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 0), StencilP_t(-1, 1, 1),
      StencilP_t( 0,-1,-1), StencilP_t( 0,-1, 0), StencilP_t( 0,-1, 1),
      StencilP_t( 0, 0,-1),                       StencilP_t( 0, 0, 1),
      StencilP_t( 0, 1,-1), StencilP_t( 0, 1, 0), StencilP_t( 0, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 0), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 0,-1), StencilP_t( 1, 0, 0), StencilP_t( 1, 0, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 0), StencilP_t( 1, 1, 1)
  );
  GlobBoundSpec_t bound_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec_1, stencil_spec_2, stencil_spec_3);

  halo_wrapper.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op_1 = halo_wrapper.stencil_operator(stencil_spec_1);
  auto stencil_op_2 = halo_wrapper.stencil_operator(stencil_spec_2);
  auto stencil_op_3 = halo_wrapper.stencil_operator(stencil_spec_3);
  auto sum_halo_spec_1 = calc_sum_halo(halo_wrapper, stencil_op_1);
  auto sum_halo_spec_2 = calc_sum_halo(halo_wrapper, stencil_op_2);
  auto sum_halo_spec_3 = calc_sum_halo(halo_wrapper, stencil_op_3);

  if(myid == 0) {
    EXPECT_EQ(sum_check_spec_1, sum_halo_spec_1);
    EXPECT_EQ(sum_check_spec_2, sum_halo_spec_2);
    EXPECT_EQ(sum_check_spec_3, sum_halo_spec_3);
  }
  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperBigMultiStencil)
{
  using Pattern_t = dash::Pattern<3>;
  using PatternCol_t = dash::Pattern<3, dash::COL_MAJOR>;
  using index_type = typename Pattern_t::index_type;
  using DistSpec_t = dash::DistributionSpec<3>;
  using Matrix_t = dash::Matrix<long, 3, index_type, Pattern_t>;
  using MatrixCol_t = dash::Matrix<long, 3, index_type, PatternCol_t>;
  using TeamSpec_t = dash::TeamSpec<3>;
  using SizeSpec_t = dash::SizeSpec<3>;
  using GlobBoundSpec_t = GlobalBoundarySpec<3>;
  using StencilP_t = StencilPoint<3>;

  auto myid(dash::myid());

  DistSpec_t dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpec_t team_spec{};
  team_spec.balance_extents();
  Pattern_t pattern(SizeSpec_t(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  Matrix_t matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  dash::Team::All().barrier();

  unsigned long sum_check_spec_1 = 0;
  unsigned long sum_check_spec_2 = 0;
  unsigned long sum_check_spec_3 = 0;

  auto ext_per_dim_check = ext_per_dim + 6;
  if(myid == 0) {
    long*** matrix_check = new long**[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long*[ext_per_dim_check];
      for(auto j = 0; j < ext_per_dim_check; ++j) {
        matrix_check[i][j] = new long[ext_per_dim_check];
        for(auto k = 0; k < ext_per_dim_check; ++k) {
          if((j == 2 || j == ext_per_dim_check - 3) && k > 2 && k < ext_per_dim_check -3) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if((j < 3 || j >= ext_per_dim_check - 3) && k > 3 && k < ext_per_dim_check - 4) {
            if(i == 0 || i == ext_per_dim - 1)
              matrix_check[i][j][k] = 10;
            else
              matrix_check[i][j][k] = 1;
            continue;
          }
          if(k < 3 || k >= ext_per_dim_check -3) {
            matrix_check[i][j][k] = 20;
            continue;
          }
          if(i == 0 || i == ext_per_dim - 1 || j == 3 || j == ext_per_dim_check - 4 ||
             k == 3 || k == ext_per_dim_check - 4) {
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j >= boundary_width + 3 && j < ext_diff + 3 &&
             k >= boundary_width + 3 && k < ext_diff + 3) {
            matrix_check[i][j][k] = 5;
            continue;
          }

          matrix_check[i][j][k] = 1;
        }
      }
    }
    for(auto j = 3; j < ext_per_dim_check - 3; ++j) {
      for(auto k = 3; k < ext_per_dim_check - 3; ++k) {
        for(auto i = 2; i < ext_per_dim - 2; ++i)
          sum_check_spec_1 += matrix_check[i][j][k] +
            matrix_check[i-2][j][k] + matrix_check[i+2][j][k] +
            matrix_check[i][j-2][k] + matrix_check[i][j+2][k] +
            matrix_check[i][j][k-2] + matrix_check[i][j][k+2];
        for(auto i = 1; i < ext_per_dim - 1; ++i)
          sum_check_spec_2 +=
            matrix_check[i-1][j-1][k-1] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-1][j+1][k-1] + matrix_check[i-1][j+1][k+1] +
            matrix_check[i][j][k] +
            matrix_check[i+1][j-1][k-1] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+1][j+1][k-1] + matrix_check[i+1][j+1][k+1];
        for(auto i = 3; i < ext_per_dim - 3; ++i)
          sum_check_spec_3 +=
            matrix_check[i-3][j-3][k-3] + matrix_check[i-2][j-2][k-2] + matrix_check[i-1][j-1][k-1] +
            matrix_check[i-3][j-3][k+3] + matrix_check[i-2][j-2][k+2] + matrix_check[i-1][j-1][k+1] +
            matrix_check[i-3][j+3][k-3] + matrix_check[i-2][j+2][k-2] + matrix_check[i-1][j+1][k-1] +
            matrix_check[i-3][j][k]     + matrix_check[i-2][j][k]     + matrix_check[i-1][j][k] +
            matrix_check[i][j-2][k]     + matrix_check[i][j][k]       + matrix_check[i][j+2][k] +
            matrix_check[i+3][j][k]     + matrix_check[i+2][j][k]     + matrix_check[i+1][j][k] +
            matrix_check[i+3][j-3][k+3] + matrix_check[i+2][j-2][k+2] + matrix_check[i+1][j-1][k+1] +
            matrix_check[i+3][j+3][k-3] + matrix_check[i+2][j+2][k-2] + matrix_check[i+1][j+1][k-1] +
            matrix_check[i+3][j+3][k+3] + matrix_check[i+2][j+2][k+2] + matrix_check[i+1][j+1][k+1];
      }
    }

    for(auto i = 0; i < ext_per_dim; ++i) {
      for(auto j = 0; j < ext_per_dim_check; ++j)
        delete[] matrix_check[i][j];
      delete[] matrix_check[i];
    }
    delete[] matrix_check;

  }

  init_matrix3D(matrix_halo);

  dash::Team::All().barrier();

  StencilSpec<StencilP_t, 6> stencil_spec_1(
      StencilP_t(-2, 0, 0), StencilP_t( 2, 0, 0),
      StencilP_t( 0,-2, 0), StencilP_t( 0, 2, 0),
      StencilP_t( 0, 0,-2), StencilP_t( 0, 0, 2)
  );
  StencilSpec<StencilP_t, 8> stencil_spec_2(
      StencilP_t(-1,-1,-1), StencilP_t(-1,-1, 1),
      StencilP_t(-1, 1,-1), StencilP_t(-1, 1, 1),
      StencilP_t( 1,-1,-1), StencilP_t( 1,-1, 1),
      StencilP_t( 1, 1,-1), StencilP_t( 1, 1, 1)
  );
  StencilSpec<StencilP_t, 26> stencil_spec_3(
      StencilP_t(-3,-3,-3), StencilP_t(-2,-2,-2), StencilP_t(-1,-1,-1),
      StencilP_t(-3,-3, 3), StencilP_t(-2,-2, 2), StencilP_t(-1,-1, 1),
      StencilP_t(-3, 3,-3), StencilP_t(-2, 2,-2), StencilP_t(-1, 1,-1),
      StencilP_t(-3, 0, 0), StencilP_t(-2, 0, 0), StencilP_t(-1, 0, 0),
      StencilP_t( 0,-2, 0),                     StencilP_t( 0, 2, 0),
      StencilP_t( 3, 0, 0), StencilP_t( 2, 0, 0), StencilP_t( 1, 0, 0),
      StencilP_t( 3,-3, 3), StencilP_t( 2,-2, 2), StencilP_t( 1,-1, 1),
      StencilP_t( 3, 3,-3), StencilP_t( 2, 2,-2), StencilP_t( 1, 1,-1),
      StencilP_t( 3, 3, 3), StencilP_t( 2, 2, 2), StencilP_t( 1, 1, 1)
  );
  GlobBoundSpec_t bound_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<Matrix_t> halo_wrapper(matrix_halo, bound_spec, stencil_spec_1, stencil_spec_2, stencil_spec_3);

  halo_wrapper.set_custom_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto stencil_op_1 = halo_wrapper.stencil_operator(stencil_spec_1);
  auto stencil_op_2 = halo_wrapper.stencil_operator(stencil_spec_2);
  auto stencil_op_3 = halo_wrapper.stencil_operator(stencil_spec_3);
  auto sum_halo_spec_1 = calc_sum_halo(halo_wrapper, stencil_op_1);
  auto sum_halo_spec_2 = calc_sum_halo(halo_wrapper, stencil_op_2);
  auto sum_halo_spec_3 = calc_sum_halo(halo_wrapper, stencil_op_3);

  auto sum_halo_spec_1_region = calc_sum_halo(halo_wrapper, stencil_op_1, true);
  auto sum_halo_spec_2_region = calc_sum_halo(halo_wrapper, stencil_op_2, true);
  auto sum_halo_spec_3_region = calc_sum_halo(halo_wrapper, stencil_op_3, true);
  if(myid == 0) {
    EXPECT_EQ(sum_check_spec_1, sum_halo_spec_1);
    EXPECT_EQ(sum_check_spec_2, sum_halo_spec_2);
    EXPECT_EQ(sum_check_spec_3, sum_halo_spec_3);
    EXPECT_EQ(sum_check_spec_1, sum_halo_spec_1_region);
    EXPECT_EQ(sum_check_spec_2, sum_halo_spec_2_region);
    EXPECT_EQ(sum_check_spec_3, sum_halo_spec_3_region);
  }

  dash::Team::All().barrier();
}

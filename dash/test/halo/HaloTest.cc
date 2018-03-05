
#include "HaloTest.h"

#include <dash/Matrix.h>
#include <dash/Algorithm.h>
#include <dash/halo/HaloMatrixWrapper.h>

#include <iostream>

using namespace dash;

TEST_F(HaloTest, GlobalBoundarySpec)
{
  using GlobBoundSpecT = GlobalBoundarySpec<3>;

  GlobBoundSpecT cycle_spec_1;
  EXPECT_EQ(cycle_spec_1[0], BoundaryProp::NONE);
  EXPECT_EQ(cycle_spec_1[1], BoundaryProp::NONE);
  EXPECT_EQ(cycle_spec_1[2], BoundaryProp::NONE);

  GlobBoundSpecT cycle_spec_2(BoundaryProp::CYCLIC, BoundaryProp::NONE, BoundaryProp::CUSTOM);
  EXPECT_EQ(cycle_spec_2[0], BoundaryProp::CYCLIC);
  EXPECT_EQ(cycle_spec_2[1], BoundaryProp::NONE);
  EXPECT_EQ(cycle_spec_2[2], BoundaryProp::CUSTOM);
}

TEST_F(HaloTest, CoordsToIndex)
{
  using RCoordsT = RegionCoords<3>;

  RCoordsT coords({1,0,1});
  EXPECT_EQ(10, coords.index());
  RCoordsT coords2({2,2,2});
  EXPECT_EQ(26, coords2.index());
}

TEST_F(HaloTest, IndexToCoords)
{
  using RCoordsT = RegionCoords<3>;

  EXPECT_EQ(RCoordsT({1,0,1}), RCoordsT(10));
  EXPECT_EQ(RCoordsT({2,2,2}), RCoordsT(26));
}

TEST_F(HaloTest, HaloSpec)
{
  using HaloRegSpecT =RegionSpec<3>;
  using HaloSpecT = HaloSpec<3>;
  using RCoordsT = RegionCoords<3>;

  HaloSpecT specs(
      HaloRegSpecT({0,0,0}, 3),
      HaloRegSpecT({0,1,1}, 2),
      HaloRegSpecT({2,1,0}, 2),
      HaloRegSpecT({2,2,2}, 3));

  EXPECT_EQ(specs.spec(0).coords(), RCoordsT({0,0,0}));
  EXPECT_EQ(specs.spec(4).coords(), RCoordsT({0,1,1}));
  EXPECT_EQ(specs.spec(21).coords(), RCoordsT({2,1,0}));
  EXPECT_EQ(specs.spec(26).coords(), RCoordsT({2,2,2}));
  EXPECT_EQ((uint32_t)specs.extent(0), 3);
  EXPECT_EQ((uint32_t)specs.extent(4), 2);
  EXPECT_EQ((uint32_t)specs.extent(21), 2);
  EXPECT_EQ((uint32_t)specs.extent(26), 3);

}

TEST_F(HaloTest, HaloSpecStencils)
{
  using HaloRegSpecT =RegionSpec<3>;
  using HaloSpecT = HaloSpec<3>;
  using RCoordsT = RegionCoords<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 6>;

  {
    StencilSpecT stencil_spec({
       StencilT(0.2d, -1,  0,  0), StencilT(0.4d, 1, 0, 0),
       StencilT( 0, -1,  0), StencilT(0, 2, 0),
       StencilT( 0,  0, -1), StencilT(0, 0, 3)});
    HaloSpecT halo_spec(stencil_spec);

    EXPECT_EQ(stencil_spec[0].coefficient(), 0.2);
    EXPECT_EQ(stencil_spec[1].coefficient(), 0.4);
    EXPECT_EQ(stencil_spec[2].coefficient(), 1.0);

    EXPECT_EQ(halo_spec.spec(4).coords(), RCoordsT({0,1,1}));
    EXPECT_EQ(halo_spec.spec(22).coords(), RCoordsT({2,1,1}));
    EXPECT_EQ(halo_spec.spec(10).coords(), RCoordsT({1,0,1}));
    EXPECT_EQ(halo_spec.spec(16).coords(), RCoordsT({1,2,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoordsT({1,1,0}));
    EXPECT_EQ(halo_spec.spec(14).coords(), RCoordsT({1,1,2}));
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    EXPECT_EQ((uint32_t)halo_spec.extent(14), 3);
  }
  {
    StencilSpecT stencil_spec({
        StencilT(0, 2, 0)});
    HaloSpecT halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(16).coords(), RCoordsT({1,2,1}));
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 2);
    for(auto i = 0; i < RCoordsT::MaxIndex; ++i) {
      if(i != 16)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    StencilSpecT stencil_spec({
        StencilT(-2, 0, -1)});
    HaloSpecT halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(3).coords(), RCoordsT({0,1,0}));
    EXPECT_EQ(halo_spec.spec(4).coords(), RCoordsT({0,1,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoordsT({1,1,0}));
    EXPECT_EQ((uint32_t)halo_spec.extent(3), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    for(auto i = 0; i < RCoordsT::MaxIndex; ++i) {
      if(i != 3 && i != 4 && i != 12)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    StencilSpecT stencil_spec({
        StencilT(-3, -2, -1)});
    HaloSpecT halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(0).coords(), RCoordsT({0,0,0}));
    EXPECT_EQ(halo_spec.spec(1).coords(), RCoordsT({0,0,1}));
    EXPECT_EQ(halo_spec.spec(3).coords(), RCoordsT({0,1,0}));
    EXPECT_EQ(halo_spec.spec(4).coords(), RCoordsT({0,1,1}));
    EXPECT_EQ(halo_spec.spec(9).coords(), RCoordsT({1,0,0}));
    EXPECT_EQ(halo_spec.spec(10).coords(), RCoordsT({1,0,1}));
    EXPECT_EQ(halo_spec.spec(12).coords(), RCoordsT({1,1,0}));
    EXPECT_EQ((uint32_t)halo_spec.extent(0), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(1), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(3), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(4), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(9), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(10), 2);
    EXPECT_EQ((uint32_t)halo_spec.extent(12), 1);
    for(auto i = 0; i < RCoordsT::MaxIndex; ++i) {
      if(i != 0 && i != 1 && i != 3 && i != 4 && i != 9 && i != 10 && i!= 12)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
  {
    StencilSpecT stencil_spec({
        StencilT(3, 3, 3)});
    HaloSpecT halo_spec(stencil_spec);

    EXPECT_EQ(halo_spec.spec(14).coords(), RCoordsT({1,1,2}));
    EXPECT_EQ(halo_spec.spec(16).coords(), RCoordsT({1,2,1}));
    EXPECT_EQ(halo_spec.spec(17).coords(), RCoordsT({1,2,2}));
    EXPECT_EQ(halo_spec.spec(22).coords(), RCoordsT({2,1,1}));
    EXPECT_EQ(halo_spec.spec(23).coords(), RCoordsT({2,1,2}));
    EXPECT_EQ(halo_spec.spec(25).coords(), RCoordsT({2,2,1}));
    EXPECT_EQ(halo_spec.spec(26).coords(), RCoordsT({2,2,2}));
    EXPECT_EQ((uint32_t)halo_spec.extent(14), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(16), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(17), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(22), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(23), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(25), 3);
    EXPECT_EQ((uint32_t)halo_spec.extent(26), 3);
    for(auto i = 0; i < RCoordsT::MaxIndex; ++i) {
      if(i != 14 && i != 16 && i != 17 && i != 22 && i != 23 && i != 25 && i!= 26)
        EXPECT_EQ((uint32_t)halo_spec.extent(i), 0);
    }
  }
}

TEST_F(HaloTest, HaloMatrixWrapperNonCyclic2D)
{
  using PatternT = dash::Pattern<2>;
  using index_type = typename PatternT::index_type;
  using MatrixT = dash::Matrix<long, 2, index_type, PatternT>;
  using DistSpecT = dash::DistributionSpec<2>;
  using TeamSpecT = dash::TeamSpec<2>;
  using SizeSpecT = dash::SizeSpec<2>;
  using GlobBoundSpecT = GlobalBoundarySpec<2>;
  using StencilT = StencilPoint<2>;
  using StencilSpecT = StencilSpec<2, 8>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);
  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  dash::Team::All().barrier();

  long sum_check = 0;
  if(myid == 0) {
    index_type ext_diff = ext_per_dim - boundary_width;

    long** matrix_check = new long*[ext_per_dim];
    for(auto i = 0; i < ext_per_dim; ++i) {
      matrix_check[i] = new long[ext_per_dim];
      for(auto j = 0; j < ext_per_dim; ++j) {
        if(i >= boundary_width  && i < ext_diff && j >= boundary_width && j < ext_diff) {
          matrix_halo[i][j] = 5;
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

  matrix_halo.barrier();

  GlobBoundSpecT cycle_spec;
  StencilSpecT stencil_spec({
      StencilT(-1,-1), StencilT(-1, 0), StencilT(-1, 1),
      StencilT( 0,-1),                  StencilT( 0, 1),
      StencilT( 1,-1), StencilT( 1, 0), StencilT( 1, 1)});

  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec, cycle_spec);

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);
  auto* sum_local = sum_halo.lbegin();

  halo_wrapper.update_async();

  auto it_iend = halo_wrapper.iend();
  for(auto it = halo_wrapper.ibegin(); it != it_iend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(i);

    *sum_local += *it;
  }

  halo_wrapper.wait();

  auto it_bend = halo_wrapper.bend();
  for(auto it = halo_wrapper.bbegin(); it != it_bend; ++it) {
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
long calc_sum_check(long*** matrix,T begin, T end) {

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

template<typename HaloWrapperT>
unsigned long calc_sum_halo(HaloWrapperT& halo_wrapper) {
  auto& stencil_spec = halo_wrapper.stencil_spec();
  halo_wrapper.update_async();

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);

  auto* sum_local = sum_halo.lbegin();
  auto it_iend = halo_wrapper.iend();
  for(auto it = halo_wrapper.ibegin(); it != it_iend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(i);

    *sum_local += *it;
  }

  halo_wrapper.wait();

  auto it_bend = halo_wrapper.bend();
  for(auto it = halo_wrapper.bbegin(); it != it_bend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(i);

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

template<typename HaloWrapperT>
unsigned long calc_sum_halo_via_stencil(HaloWrapperT& halo_wrapper) {
  auto& stencil_spec = halo_wrapper.stencil_spec();
  halo_wrapper.update_async();

  dash::Array<long> sum_halo(dash::size());
  dash::fill(sum_halo.begin(), sum_halo.end(),0);

  auto* sum_local = sum_halo.lbegin();
  auto it_iend = halo_wrapper.iend();
  for(auto it = halo_wrapper.ibegin(); it != it_iend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
      *sum_local += it.value_at(stencil_spec[i]);

    *sum_local += *it;
  }

  halo_wrapper.wait();

  auto it_bend = halo_wrapper.bend();
  for(auto it = halo_wrapper.bbegin(); it != it_bend; ++it) {
    for(auto i = 0; i < stencil_spec.num_stencil_points(); ++i)
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

TEST_F(HaloTest, HaloMatrixWrapperNonCyclic3D)
{
  using PatternT = dash::Pattern<3>;
  using index_type = typename PatternT::index_type;
  using MatrixT = dash::Matrix<long, 3, index_type, PatternT>;
  using DistSpecT = dash::DistributionSpec<3>;
  using TeamSpecT = dash::TeamSpec<3>;
  using SizeSpecT = dash::SizeSpec<3>;
  using GlobBoundSpecT = GlobalBoundarySpec<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 26>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  index_type ext_diff = ext_per_dim - boundary_width;
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
            matrix_halo[i][j][k] = 5;
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

  matrix_halo.barrier();

  StencilSpecT stencil_spec({
      StencilT(-1,-1,-1), StencilT(-1,-1, 0), StencilT(-1,-1, 1),
      StencilT(-1, 0,-1), StencilT(-1, 0, 0), StencilT(-1, 0, 1),
      StencilT(-1, 1,-1), StencilT(-1, 1, 0), StencilT(-1, 1, 1),
      StencilT( 0,-1,-1), StencilT( 0,-1, 0), StencilT( 0,-1, 1),
      StencilT( 0, 0,-1),                     StencilT( 0, 0, 1),
      StencilT( 0, 1,-1), StencilT( 0, 1, 0), StencilT( 0, 1, 1),
      StencilT( 1,-1,-1), StencilT( 1,-1, 0), StencilT( 1,-1, 1),
      StencilT( 1, 0,-1), StencilT( 1, 0, 0), StencilT( 1, 0, 1),
      StencilT( 1, 1,-1), StencilT( 1, 1, 0), StencilT( 1, 1, 1)
  });

  GlobBoundSpecT cycle_spec;
  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec);
  auto sum_halo = calc_sum_halo(halo_wrapper);
  if(myid == 0) {
    // global outer boundary not included in both sums
    EXPECT_EQ(sum_check, sum_halo);
  }
  dash::Team::All().barrier();

}


TEST_F(HaloTest, HaloMatrixWrapperCyclic3D)
{
  using PatternT = dash::Pattern<3>;
  using PatternColT = dash::Pattern<3, dash::COL_MAJOR>;
  using index_type = typename PatternT::index_type;
  using DistSpecT = dash::DistributionSpec<3>;
  using MatrixT = dash::Matrix<long, 3, index_type, PatternT>;
  using MatrixColT = dash::Matrix<long, 3, index_type, PatternColT>;
  using TeamSpecT = dash::TeamSpec<3>;
  using SizeSpecT = dash::SizeSpec<3>;
  using GlobBoundSpecT = GlobalBoundarySpec<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 26>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());
  PatternColT pattern_col(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);
  MatrixColT matrix_halo_col(pattern_col);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);
  dash::fill(matrix_halo_col.begin(), matrix_halo_col.end(), 1);

  dash::Team::All().barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  index_type ext_diff = ext_per_dim - boundary_width;
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
            matrix_halo[i-1][j-1][k-1] = 10;
            matrix_halo_col[i-1][j-1][k-1] = 10;
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i > boundary_width  && i <= ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_halo[i-1][j-1][k-1] = 5;
            matrix_halo_col[i-1][j-1][k-1] = 5;
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

  dash::Team::All().barrier();

  StencilSpecT stencil_spec({
      StencilT(-1,-1,-1), StencilT(-1,-1, 0), StencilT(-1,-1, 1),
      StencilT(-1, 0,-1), StencilT(-1, 0, 0), StencilT(-1, 0, 1),
      StencilT(-1, 1,-1), StencilT(-1, 1, 0), StencilT(-1, 1, 1),
      StencilT( 0,-1,-1), StencilT( 0,-1, 0), StencilT( 0,-1, 1),
      StencilT( 0, 0,-1),                     StencilT( 0, 0, 1),
      StencilT( 0, 1,-1), StencilT( 0, 1, 0), StencilT( 0, 1, 1),
      StencilT( 1,-1,-1), StencilT( 1,-1, 0), StencilT( 1,-1, 1),
      StencilT( 1, 0,-1), StencilT( 1, 0, 0), StencilT( 1, 0, 1),
      StencilT( 1, 1,-1), StencilT( 1, 1, 0), StencilT( 1, 1, 1)
  });
  GlobBoundSpecT cycle_spec(BoundaryProp::CYCLIC, BoundaryProp::CYCLIC, BoundaryProp::CYCLIC);
  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec, cycle_spec);
  HaloMatrixWrapper<MatrixColT,StencilSpecT> halo_wrapper_col(matrix_halo_col, stencil_spec, cycle_spec);


  auto sum_halo = calc_sum_halo(halo_wrapper);
  auto sum_halo_col = calc_sum_halo(halo_wrapper_col);
  if(myid == 0) {
    for(auto i = 0; i < ext_per_dim; ++i)
      for(auto j = 0; j < ext_per_dim; ++j)
        for(auto k = 0; k < ext_per_dim; ++k)
          EXPECT_EQ((long)matrix_halo[i][j][k], (long)matrix_halo_col[i][j][k]);
  }

  if(myid == 0) {
    EXPECT_EQ(sum_check, sum_halo);
    EXPECT_EQ(sum_check, sum_halo_col);
  }
  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperFixed3D)
{
  using PatternT = dash::Pattern<3>;
  using index_type = typename PatternT::index_type;
  using DistSpecT = dash::DistributionSpec<3>;
  using MatrixT = dash::Matrix<long, 3, index_type, PatternT>;
  using TeamSpecT = dash::TeamSpec<3>;
  using SizeSpecT = dash::SizeSpec<3>;
  using GlobBoundSpecT = GlobalBoundarySpec<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 26>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  index_type ext_diff = ext_per_dim - boundary_width;
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
            matrix_halo[i-1][j-1][k-1] = 10;
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i > boundary_width  && i <= ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_halo[i-1][j-1][k-1] = 5;
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


  matrix_halo.barrier();

  StencilSpecT stencil_spec({
      StencilT(-1,-1,-1), StencilT(-1,-1, 0), StencilT(-1,-1, 1),
      StencilT(-1, 0,-1), StencilT(-1, 0, 0), StencilT(-1, 0, 1),
      StencilT(-1, 1,-1), StencilT(-1, 1, 0), StencilT(-1, 1, 1),
      StencilT( 0,-1,-1), StencilT( 0,-1, 0), StencilT( 0,-1, 1),
      StencilT( 0, 0,-1),                     StencilT( 0, 0, 1),
      StencilT( 0, 1,-1), StencilT( 0, 1, 0), StencilT( 0, 1, 1),
      StencilT( 1,-1,-1), StencilT( 1,-1, 0), StencilT( 1,-1, 1),
      StencilT( 1, 0,-1), StencilT( 1, 0, 0), StencilT( 1, 0, 1),
      StencilT( 1, 1,-1), StencilT( 1, 1, 0), StencilT( 1, 1, 1)
  });
  GlobBoundSpecT cycle_spec(BoundaryProp::CUSTOM, BoundaryProp::CUSTOM, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec, cycle_spec);

  halo_wrapper.set_fixed_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto sum_halo = calc_sum_halo(halo_wrapper);

  if(myid == 0)
    EXPECT_EQ(sum_check, sum_halo);

  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperMix3D)
{
  using PatternT = dash::Pattern<3>;
  using index_type = typename PatternT::index_type;
  using DistSpecT = dash::DistributionSpec<3>;
  using MatrixT = dash::Matrix<long, 3, index_type, PatternT>;
  using TeamSpecT = dash::TeamSpec<3>;
  using SizeSpecT = dash::SizeSpec<3>;
  using GlobBoundSpecT = GlobalBoundarySpec<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 26>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);

  matrix_halo.barrier();

  auto ext_per_dim_check = ext_per_dim + 2;
  unsigned long sum_check = 0;
  index_type ext_diff = ext_per_dim - boundary_width;
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
            matrix_halo[i][j-1][k-1] = 10;
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j > boundary_width && j <= ext_diff &&
             k > boundary_width && k <= ext_diff) {
            matrix_halo[i][j-1][k-1] = 5;
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

  matrix_halo.barrier();

  StencilSpecT stencil_spec({
      StencilT(-1,-1,-1), StencilT(-1,-1, 0), StencilT(-1,-1, 1),
      StencilT(-1, 0,-1), StencilT(-1, 0, 0), StencilT(-1, 0, 1),
      StencilT(-1, 1,-1), StencilT(-1, 1, 0), StencilT(-1, 1, 1),
      StencilT( 0,-1,-1), StencilT( 0,-1, 0), StencilT( 0,-1, 1),
      StencilT( 0, 0,-1),                     StencilT( 0, 0, 1),
      StencilT( 0, 1,-1), StencilT( 0, 1, 0), StencilT( 0, 1, 1),
      StencilT( 1,-1,-1), StencilT( 1,-1, 0), StencilT( 1,-1, 1),
      StencilT( 1, 0,-1), StencilT( 1, 0, 0), StencilT( 1, 0, 1),
      StencilT( 1, 1,-1), StencilT( 1, 1, 0), StencilT( 1, 1, 1)
  });
  GlobBoundSpecT cycle_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec, cycle_spec);

  halo_wrapper.set_fixed_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto sum_halo = calc_sum_halo(halo_wrapper);

  if(myid == 0)
    EXPECT_EQ(sum_check, sum_halo);

  dash::Team::All().barrier();
}

TEST_F(HaloTest, HaloMatrixWrapperBigMix3D)
{
  using PatternT = dash::Pattern<3>;
  using PatternColT = dash::Pattern<3, dash::COL_MAJOR>;
  using index_type = typename PatternT::index_type;
  using DistSpecT = dash::DistributionSpec<3>;
  using MatrixT = dash::Matrix<long, 3, index_type, PatternT>;
  using MatrixColT = dash::Matrix<long, 3, index_type, PatternColT>;
  using TeamSpecT = dash::TeamSpec<3>;
  using SizeSpecT = dash::SizeSpec<3>;
  using GlobBoundSpecT = GlobalBoundarySpec<3>;
  using StencilT = StencilPoint<3>;
  using StencilSpecT = StencilSpec<3, 26>;

  auto myid(dash::myid());

  auto boundary_width = 5;

  DistSpecT dist_spec(dash::BLOCKED, dash::BLOCKED, dash::BLOCKED);
  TeamSpecT team_spec{};
  team_spec.balance_extents();
  PatternT pattern(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());
  PatternColT pattern_col(SizeSpecT(ext_per_dim,ext_per_dim,ext_per_dim), dist_spec, team_spec, dash::Team::All());

  MatrixT matrix_halo(pattern);
  MatrixColT matrix_halo_col(pattern_col);

  dash::fill(matrix_halo.begin(), matrix_halo.end(), 1);
  dash::fill(matrix_halo_col.begin(), matrix_halo_col.end(), 1);

  dash::Team::All().barrier();

  auto ext_per_dim_check = ext_per_dim + 6;
  unsigned long sum_check = 0;
  index_type ext_diff = ext_per_dim - boundary_width;
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
            if(j >= 3 && k >= 3 && j < ext_per_dim_check - 3 && k < ext_per_dim_check - 3) {
              matrix_halo[i][j-3][k-3] = 10;
              matrix_halo_col[i][j-3][k-3] = 10;
            }
            matrix_check[i][j][k] = 10;
            continue;
          }
          if(i >= boundary_width  && i < ext_diff && j >= boundary_width + 3 && j < ext_diff + 3 &&
             k >= boundary_width + 3 && k < ext_diff + 3) {
            matrix_halo[i][j-3][k-3] = 5;
            matrix_halo_col[i][j-3][k-3] = 5;
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

  dash::Team::All().barrier();

  StencilSpecT stencil_spec({
      StencilT(-3,-3,-3), StencilT(-2,-2,-2), StencilT(-1,-1,-1),
      StencilT(-3,-3, 3), StencilT(-2,-2, 2), StencilT(-1,-1, 1),
      StencilT(-3, 3,-3), StencilT(-2, 2,-2), StencilT(-1, 1,-1),
      StencilT(-3, 0, 0), StencilT(-2, 0, 0), StencilT(-1, 0, 0),
      StencilT( 0,-2, 0),                     StencilT( 0, 2, 0),
      StencilT( 3, 0, 0), StencilT( 2, 0, 0), StencilT( 1, 0, 0),
      StencilT( 3,-3, 3), StencilT( 2,-2, 2), StencilT( 1,-1, 1),
      StencilT( 3, 3,-3), StencilT( 2, 2,-2), StencilT( 1, 1,-1),
      StencilT( 3, 3, 3), StencilT( 2, 2, 2), StencilT( 1, 1, 1)
  });
  GlobBoundSpecT cycle_spec(BoundaryProp::NONE, BoundaryProp::CYCLIC, BoundaryProp::CUSTOM);
  HaloMatrixWrapper<MatrixT,StencilSpecT> halo_wrapper(matrix_halo, stencil_spec, cycle_spec);
  HaloMatrixWrapper<MatrixColT,StencilSpecT> halo_wrapper_col(matrix_halo_col, stencil_spec, cycle_spec);

  halo_wrapper.set_fixed_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });

  auto sum_halo = calc_sum_halo(halo_wrapper);

  auto sum_halo_via_stencil = calc_sum_halo_via_stencil(halo_wrapper);

  halo_wrapper_col.set_fixed_halos([](const std::array<dash::default_index_t,3>& coords) {
      return 20;
  });
  auto sum_halo_col = calc_sum_halo(halo_wrapper_col);

  if(myid == 0) {
    EXPECT_EQ(sum_check, sum_halo);
    EXPECT_EQ(sum_check, sum_halo_via_stencil);
    EXPECT_EQ(sum_check, sum_halo_col);
  }
  dash::Team::All().barrier();
}

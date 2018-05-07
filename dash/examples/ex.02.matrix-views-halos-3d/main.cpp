#include<iostream>

#include <libdash.h>
#include <dash/View.h>

using namespace std;

static constexpr dash::dim_t NumDimensions = 3;

template<class MatrixT>
void print_matrix(const MatrixT & matrix)
{
  string indent = " ";
  auto nrows = matrix.extent(0);
  auto ncols = matrix.extent(1);
  auto nlayers = matrix.extent(2);

  cout << "Matrix:" << endl;
  for (auto l = 0; l < nlayers; ++l) {
    cout << indent << "layer: " << l << endl;
    for (auto r = 0; r < nrows; ++r) {
      cout << indent;
      for (auto c = 0; c < ncols; ++c) {
        //cout << " " << setw(2) << (int)matrix[r][c] << "(" << setw(3) << matrix.pattern().global_at({{ r,c }}) << ")";
        cout << " " << setw(4) << (int)matrix[r][c][l] << "(" << setw(1) << matrix.pattern().unit_at({{ r,c,l }}) << ")";
      }
      cout << endl;
    }
    indent += " ";
  }
}

template<class ViewT>
void print_view(const ViewT & view, string name)
{
  string indent = " ";
  auto nrows = view.extent(0);
  auto ncols = view.extent(1);
  auto nlayers = view.extent(2);

  cout << "View: " << name << endl;
  for (auto l = 0; l < nlayers; ++l) {
    cout << indent << "layer: " << l << endl;
    for (auto r = 0; r < nrows; ++r) {
      cout << indent;
      for (auto c = 0; c < ncols; ++c) {
        auto offset = ncols * (l * nrows + r) + c;
        cout << " " << setw(3) << (long)view.begin()[offset];
      }
      cout << endl;
    }
    indent += " ";
  }
}

template<class ViewT>
void print_view_index(const ViewT & view, string name)
{
  string indent = " ";
  auto nrows = view.extent(0);
  auto ncols = view.extent(1);
  auto nlayers = view.extent(2);

  cout << "View: " << name << endl;
  for (auto l = 0; l < nlayers; ++l) {
    cout << indent << "layer: " << l << endl;
    for (auto r = 0; r < nrows; ++r) {
      cout << indent;
      for (auto c = 0; c < ncols; ++c) {
        auto offset = ncols * (l * nrows + r) + c;
        auto it = view.begin() + offset;
        cout << " " << setw(5) << (long)*it << "(" << it.gpos() << ")";
      }
      cout << endl;
    }
    indent += " ";
  }
}
/*template<dash::dim_t dim, typename T, typename Arg>
constexpr decltype(auto) my_expand(T&& mat, Arg arg) {
  return  dash::expand<dim>(arg.first,arg.second, mat);
}

template<dash::dim_t dim, typename T, typename Arg, typename... Args>
constexpr decltype(auto) my_expand(T&& mat, Arg arg, Args... args) {
  auto&& tmp = dash::expand<dim>(arg.first,arg.second,mat);
  return  my_expand<dim+1>(tmp, args...);
}*/

template<dash::dim_t dim>
struct my_expand {
  template<typename T>
  static constexpr auto  foo(T&& mat, const std::array<std::pair<int,int>,NumDimensions>& args) {
    constexpr dash::dim_t dim_tmp = NumDimensions - 1 - dim;
    auto tmp = dash::expand<dim_tmp>(std::get<dim_tmp>(args).first,std::get<dim_tmp>(args).second, mat);

    return my_expand<dim-1>::foo(tmp,args);
  }
};

template<>
struct my_expand<0> {
  template<typename T>
  static constexpr auto foo(T&& mat, const std::array<std::pair<int,int>,NumDimensions>& args) {
    constexpr dash::dim_t dim_tmp = NumDimensions - 1;
    return dash::expand<dim_tmp>(std::get<dim_tmp>(args).first,std::get<dim_tmp>(args).second, mat);
  }
};

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  using pattern_t   = dash::Pattern<NumDimensions>;
  using size_spec_t = dash::SizeSpec<NumDimensions>;
  using dist_spec_t = dash::DistributionSpec<NumDimensions>;
  using team_spec_t = dash::TeamSpec<NumDimensions>;
  using StencilPoint_t = dash::halo::StencilPoint<NumDimensions>;
  using StencilSpec_t = dash::halo::StencilSpec<StencilPoint_t,6>;
  using HaloSpec_t = dash::halo::HaloSpec<NumDimensions>;

  auto myid = dash::myid();
  auto num_units = dash::size();
  pattern_t pattern(20,10,5);
  dash::Matrix<int, NumDimensions, typename pattern_t::index_type, pattern_t> matrix(pattern);

  StencilSpec_t stencil_spec(
      StencilPoint_t(-1, 0, 0), StencilPoint_t( 1, 0, 0),
      StencilPoint_t( 0,-1, 0), StencilPoint_t( 0, 1, 0),
      StencilPoint_t( 0, 0,-1), StencilPoint_t( 0, 0,-1)
  );
  HaloSpec_t halo_spec(stencil_spec);
  auto dist = stencil_spec.minmax_distances();

  // setup matrix
  if(myid == 0) {
    auto count = 0;
    for(auto i = 0; i < matrix.extent(0); ++i)
      for(auto j = 0; j < matrix.extent(1); ++j)
        for(auto k = 0; k < matrix.extent(2); ++k)
          matrix[i][j][k] = count++;
  }

  auto view_local = matrix | dash::local() | dash::block(0);

  // build inner view
  std::array<std::pair<int,int>,NumDimensions> values_inner;
  for(auto d = 0; d < NumDimensions; ++d) {
    values_inner[d] = std::make_pair(-dist[d].first,-dist[d].second);
  }
  auto view_inner = my_expand<NumDimensions-1>::foo(view_local, values_inner);

  // build northern boundary view
  std::array<std::pair<int,int>,NumDimensions> values_bnd_north;
  auto view_bnd_north = dash::sub<0>(0, -dist[0].first, view_local);
  auto view_bnd_south = view_bnd_north | dash::shift<0>(view_local.extent(0) - dist[0].second );
  //values_bnd_north[0] = std::make_pair(0, -(view_local.extent(0) + dist[0].first) );
  //auto view_bnd_north = my_expand<1>::foo(view_local, values_bnd_north);
  auto view_halo_north= view_bnd_north | dash::shift<0>(dist[0].first);

  auto view_bnd_west =
    dash::sub<0>(-dist[0].first, view_local.extent(0) - dist[0].second, view_local) |
    dash::sub<1>(0, -dist[1].first);
  auto view_bnd_east = view_bnd_west | dash::shift<1>(view_local.extent(1) - dist[1].second );

  auto view_bnd_front =
    dash::sub<0>(-dist[0].first, view_local.extent(0) - dist[0].second, view_local) |
    dash::sub<1>(-dist[1].first, view_local.extent(1) - dist[1].second) |
    dash::sub<2>(0, -dist[2].first);
  auto view_bnd_back  = view_bnd_front | dash::shift<2>(view_local.extent(2) - dist[2].second );

  matrix.barrier();

  if(myid == 2) {

    print_matrix(matrix);
    print_view_index(view_local, "local");
    print_view_index(view_inner, "inner");
    print_view_index(view_bnd_north, "bnd north");
    print_view_index(view_bnd_south, "bnd south");
    print_view_index(view_bnd_west, "bnd west");
    print_view_index(view_bnd_east, "bnd east");
  }

  matrix.barrier();

  if(myid == 2) {

    print_view(view_halo_north, "halo north");
  }

  dash::finalize();

  return 0;
}

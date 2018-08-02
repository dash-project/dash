#include <iostream>
#include <libdash.h>

using namespace std;

using pattern_t    = dash::Pattern<2>;
using size_spec_t  = dash::SizeSpec<2>;
using dist_spec_t  = dash::DistributionSpec<2>;
using team_spec_t  = dash::TeamSpec<2>;
using matrix_t     = dash::Matrix<
                       double, 2,
                       typename pattern_t::index_type,
                       pattern_t>;
using StencilT     = dash::halo::StencilPoint<2>;
using StencilSpecT = dash::halo::StencilSpec<StencilT,4>;
using GlobBoundSpecT   = dash::halo::GlobalBoundarySpec<2>;
using HaloMatrixWrapperT = dash::halo::HaloMatrixWrapper<matrix_t>;

using array_t      = dash::Array<double>;

void print_matrix(const matrix_t& matrix) {
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);
  cout << fixed;
  cout.precision(4);
  cout << "Matrix:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c)
      cout << " " << setw(3) << (double) matrix[r][c];
    cout << endl;
  }
}

double calcEnergy(const matrix_t& m, array_t &a) {
  *a.lbegin() = std::accumulate( m.lbegin(), m.lend(), 0.0);
  a.barrier();

  if(dash::myid() == 0)
    return std::accumulate(a.begin(), a.end(), 0.0);
  else
    return 0.0;
}

int main(int argc, char *argv[])
{

  if (argc < 3) {
    cerr << "Not enough arguments ./<prog> matrix_ext iterations" << endl;
    return 1;
  }
  auto matrix_ext = std::atoi(argv[1]);
  auto iterations = std::atoi(argv[2]);

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto ranks = dash::size();

  dist_spec_t dist(dash::BLOCKED, dash::BLOCKED);
  team_spec_t tspec(ranks,1);

  tspec.balance_extents();

  pattern_t pattern(size_spec_t(matrix_ext, matrix_ext), dist, tspec,
                    dash::Team::All());

  matrix_t matrix(pattern);
  matrix_t matrix2(pattern);

  if (myid == 0) {
    std::fill(matrix.lbegin(), matrix.lend(), 1.0);
    std::fill(matrix2.lbegin(), matrix2.lend(), 1.0);
  } else {
    std::fill(matrix.lbegin(), matrix.lend(), 0.0);
    std::fill(matrix2.lbegin(), matrix2.lend(), 0.0);
  }

  matrix.barrier();

#ifdef DEBUG
  if (myid == 0)
    print_matrix(matrix);
#endif

  StencilSpecT stencil_spec( StencilT(-1, 0), StencilT(1, 0), StencilT( 0, -1), StencilT(0, 1));

  GlobBoundSpecT bound_spec(dash::halo::BoundaryProp::CYCLIC, dash::halo::BoundaryProp::CYCLIC);

  HaloMatrixWrapperT halomat(matrix, bound_spec, stencil_spec);
  HaloMatrixWrapperT halomat2(matrix2, bound_spec, stencil_spec);

  auto stencil_op = halomat.stencil_operator(stencil_spec);
  auto stencil_op2 = halomat2.stencil_operator(stencil_spec);

  auto* current_op = &stencil_op;
  auto* new_op = &stencil_op2;

  HaloMatrixWrapperT* current_halo = &halomat;
  HaloMatrixWrapperT* new_halo = &halomat2;



  double dx{ 1.0 };
  double dy{ 1.0 };
  double dt{ 0.05 };
  double k{ 1.0 };

  // initial total energy
  array_t energy(ranks);
  double initEnergy = calcEnergy(current_halo->matrix(), energy);

  const auto &lview = halomat.view_local();
  auto offset = lview.extent(1);
  long inner_start = offset + 1;
  long inner_end = lview.extent(0) * (offset - 1) - 1;

  current_halo->matrix().barrier();

  for (auto d = 0; d < iterations; ++d) {

    auto& current_matrix = current_halo->matrix();
    auto& new_matrix = new_halo->matrix();

    // Update Halos asynchroniously
    current_halo->update_async();

    // optimized calculation of inner matrix elements
    auto* current_begin = current_matrix.lbegin();
    auto* new_begin = new_matrix.lbegin();
#if 0
    for (auto i = inner_start; i < inner_end; i += offset) {
      auto* center = current_begin + i;
      auto* center_y_plus = center + offset;
      auto* center_y_minus = center - offset;
      for (auto j = 0; j < offset - 2;
           ++j, ++center, ++center_y_plus, ++center_y_minus) {
        /*auto dtheta =
            (*(center - 1) + *(center + 1) - 2 * (*center)) / (dx * dx) +
            (*(center_y_minus) + *(center_y_plus) - 2 * (*center)) / (dy * dy);
        *(new_begin + i + j) = *center + k * dtheta * dt;*/
        *(new_begin + i + j) = calc(center, center_y_minus, center_y_plus, center - 1, center + 1);
      }
    }
#endif
    // slow version
    auto it_end = current_op->inner.end();
    for(auto it = current_op->inner.begin(); it != it_end; ++it)
    {
      auto core = *it;
      auto dtheta = (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
                    (it.value_at(2) + it.value_at(3) - 2 * core) /(dy * dy);
      new_begin[it.lpos()] = core + k * dtheta * dt;
    }

    // Wait until all Halo updates ready
    current_halo->wait();

    // Calculation of boundary Halo elements
    auto it_bend = current_op->boundary.end();
    for (auto it = current_op->boundary.begin(); it != it_bend; ++it) {
      auto core = *it;
      double dtheta =
          (it.value_at(0) + it.value_at(1) - 2 * core) / (dx * dx) +
          (it.value_at(2) + it.value_at(3) - 2 * core) / (dy * dy);
      new_begin[it.lpos()] = core + k * dtheta * dt;
    }

    // swap current matrix and current halo matrix
    std::swap(current_halo, new_halo);
    std::swap(current_op, new_op);
    current_matrix.barrier();
  }
  // final total energy
  double endEnergy = calcEnergy(current_halo->matrix(), energy);

#ifdef DEBUG
  if (myid == 0)
    print_matrix(current_halo->matrix());
#endif

  // Output
  if (myid == 0) {
    cout << fixed;
    cout.precision(5);
    cout << "InitEnergy=" << initEnergy << endl;
    cout << "EndEnergy=" << endEnergy << endl;
    cout << "DiffEnergy=" << endEnergy - initEnergy << endl;
    cout << "Matrixspec: " << matrix_ext << " x " << matrix_ext << endl;
    cout << "Iterations: " << iterations << endl;
    cout.flush();
  }

  dash::finalize();

  return 0;
}

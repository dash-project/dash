#include <iostream>
#include <chrono>
#include <libdash.h>
#include <iomanip>
#include <utility>

#include <dash/experimental/HaloMatrix.h>

using namespace std;

using pattern_t    = dash::Pattern<2>;
using size_spec_t  = dash::SizeSpec<2>;
using dist_spec_t  = dash::DistributionSpec<2>;
using team_spec_t  = dash::TeamSpec<2>;
using matrix_t     = dash::Matrix<
                       double, 2,
                       typename pattern_t::index_type,
                       pattern_t>;
using array_t      = dash::Array<double>;
using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
using time_diff_t  = std::chrono::duration<double>;

void print_matrix(const matrix_t *matrix) {
  auto rows = matrix->extent(0);
  auto cols = matrix->extent(1);
  cout << fixed;
  cout.precision(1);
  cout << "Matrix:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c)
      cout << " " << setw(3) << (*matrix)[r][c];
    cout << endl;
  }
}

double calcEnergy(const matrix_t *m, array_t &a) {
  a[dash::myid()] = std::accumulate(
                      (*m).local.lbegin(),
                      (*m).local.lend(),
                      0.0);
  m->barrier();
  return dash::accumulate(a.begin(), a.end(), 0.0);
}

int main(int argc, char *argv[])
{
  using dash::experimental::HaloSpec;
  using dash::experimental::HaloMatrix;

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
  team_spec_t tspec;

  tspec.balance_extents();

  pattern_t pattern(size_spec_t(matrix_ext, matrix_ext), dist, tspec,
                    dash::Team::All());

  matrix_t matrix(pattern);
  matrix_t matrix2(pattern);

  if (myid == 0) {
    std::fill(matrix.local.lbegin(), matrix.local.lend(), 1.0);
    std::fill(matrix2.local.lbegin(), matrix2.local.lend(), 1.0);
  } else {
    std::fill(matrix.local.lbegin(), matrix.local.lend(), 0.0);
    std::fill(matrix2.local.lbegin(), matrix2.local.lend(), 0.0);
  }

  matrix_t *current_matrix = &matrix;
  matrix_t *new_matrix = &matrix2;

  matrix.barrier();

#ifdef DEBUG
  if (myid == 0)
    print_matrix(current_matrix);
#endif

  HaloSpec<2> halo_s({ { { -1, 1 }, { -1, 1 } } });

  HaloMatrix<matrix_t, HaloSpec<2> > halomat(matrix, halo_s);
  HaloMatrix<matrix_t, HaloSpec<2> > halomat2(matrix2, halo_s);

  HaloMatrix<matrix_t, HaloSpec<2> > *current_halo = &halomat;
  HaloMatrix<matrix_t, HaloSpec<2> > *new_halo = &halomat2;

  time_point_t s_total, e_total;

  double dx{ 1.0 };
  double dy{ 1.0 };
  double dt{ 0.05 };
  double k{ 1.0 };

  // initial total energy
  array_t energy(ranks);
  double initEnergy = calcEnergy(&matrix, energy);

  const auto &lview = halomat.getLocalView();
  auto offset = lview.extent(1);
  long inner_start = offset + 1;
  long inner_end = lview.extent(0) * (offset - 1) - 1;

  // start time measurement
  s_total = std::chrono::system_clock::now();
  for (auto d = 0; d < iterations; ++d) {
    // Update Halos asynchroniously
    current_halo->updateHalosAsync();

    // optimized calculation of inner matrix elements
    auto current_begin = current_matrix->local.lbegin();
    auto new_begin = new_matrix->local.lbegin();
    for (auto i = inner_start; i < inner_end; i += offset) {
      auto center = current_begin + i;
      auto center_y_plus = center + offset;
      auto center_y_minus = center - offset;
      for (auto j = 0; j < offset - 2;
           ++j, ++center, ++center_y_plus, ++center_y_minus) {
        auto dtheta =
            (*(center - 1) + *(center + 1) - 2 * (*center)) / (dx * dx) +
            (*(center_y_minus) + *(center_y_plus) - 2 * (*center)) / (dy * dy);
        *(new_begin + i + j) = *center + k * dtheta * dt;
      }
    }

    // slow version
    /*for(auto it = current_halo->ibegin(); it != current_halo->iend(); ++it)
    {
      auto core = *it;
      auto dtheta = (it.halo_value(1, -1) + it.halo_value(1, 1) - 2 * core) /
    (dx * dx) +
                    (it.halo_value(0, -1) + it.halo_value(0, 1) - 2 * core) /
    (dy * dy);
      *(new_begin + it.lpos()) = core + k * dtheta * dt;
    }*/

    // Wait until all Halo updates ready
    current_halo->waitHalosAsync();

    // Calculation of boundary Halo elements
    for (auto it = current_halo->bbegin(); it != current_halo->bend(); ++it) {
      auto core = *it;
      double dtheta =
          (it.halo_value(1, -1) + it.halo_value(1, 1) - 2 * core) / (dx * dx) +
          (it.halo_value(0, -1) + it.halo_value(0, 1) - 2 * core) / (dy * dy);
      *(new_begin + it.lpos()) = core + k * dtheta * dt;
    }

    // swap current matrix and current halo matrix
    std::swap(current_matrix, new_matrix);
    std::swap(current_halo, new_halo);

    new_matrix->barrier();
  }
  // finish time measurement
  e_total = std::chrono::system_clock::now();
  double diff_total = time_diff_t(e_total - s_total).count();

  // final total energy
  double endEnergy = calcEnergy(new_matrix, energy);

#ifdef DEBUG
  if (myid == 0)
    print_matrix(&matrix);
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

  for (auto id = 0; id < ranks; ++id) {
    dash::Team::All().barrier();
    if (id == myid) {
      cout << "ID: " << id << " - Total Time: " << diff_total
           << " - iteration avg: " << diff_total / iterations << endl;
      cout.flush();
    }
  }

  dash::finalize();

  return 0;
}

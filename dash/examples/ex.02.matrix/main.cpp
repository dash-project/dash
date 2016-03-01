/*
 * hello/main.cpp
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <iomanip>

#include <libdash.h>

using std::cout;
using std::endl;
using std::setw;

template<class MatrixT>
void print_matrix(const MatrixT & matrix) {
  typedef typename MatrixT::value_type value_t;
  auto extent_cols = matrix.extent(0);
  auto extent_rows = matrix.extent(1);

  // Creating local copy for output to prevent interleaving with log
  // messages:
  value_t * matrix_copy = new value_t[matrix.size()];
  auto copy_end = std::copy(matrix.begin(),
                            matrix.end(),
                            matrix_copy);
  DASH_ASSERT(copy_end == matrix_copy + matrix.size());
  cout << "Matrix:" << endl;
  for (auto r = 0; r < extent_rows; ++r) {
    for (auto c = 0; c < extent_cols; ++c) {
      cout << " " << setw(5) << matrix_copy[r * extent_cols + c];
    }
    cout << endl;
  }
  delete[] matrix_copy;
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 2;
  size_t tilesize_y  = 3;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;
  dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           extent_cols,
                           extent_rows),
                         dash::DistributionSpec<2>(
                           dash::TILE(tilesize_x),
                           dash::TILE(tilesize_y)));
  size_t matrix_size = extent_cols * extent_rows;
  DASH_ASSERT(matrix_size == matrix.size());
  DASH_ASSERT(extent_cols == matrix.extent(0));
  DASH_ASSERT(extent_rows == matrix.extent(1));

  cout << "Matrix size: " << extent_cols
       << " x " << extent_rows
       << " == " << matrix_size
       << endl;

  // Fill matrix
  if (0 == myid) {
    cout << "Assigning matrix values" << endl;
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        matrix[i][k] = (i * 11) + (k * 97);
      }
    }
  }

  // Units waiting for value initialization
  dash::Team::All().barrier();

  // Read and assert values in matrix
  for (size_t i = 0; i < matrix.extent(0); ++i) {
    for (size_t k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      DASH_ASSERT(expected == value);
    }
  }

  dash::Team::All().barrier();

  // print matrix
  if (0 == myid) {
    print_matrix(matrix);
  }

  dash::Team::All().barrier();

  std::fill(matrix.lbegin(), matrix.lend(), myid);

  matrix[2][2 + myid] = 42;

  dash::Team::All().barrier();

  // print matrix
  if (0 == myid) {
    print_matrix(matrix);
  }

  dash::Team::All().barrier();

  if (0 == myid) {
    auto mixed_range = matrix.rows(7,4).cols(1,6);
    std::fill(mixed_range.begin(), mixed_range.end(), 8888);

    auto local_range = matrix.local.block(1);
    std::fill(local_range.begin(), local_range.end(), 1111);

    auto remote_range = matrix.block(dash::Team::All().size() - 1);
    std::fill(remote_range.begin(), remote_range.end(), 4444);
  }

  dash::Team::All().barrier();

  // print matrix
  if (0 == myid) {
    print_matrix(matrix);
  }

  dash::finalize();
}

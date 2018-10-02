#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <iomanip>

#include <libdash.h>

using std::cout;
using std::endl;
using std::setw;

template<class MatrixT>
void print_matrix_1(const MatrixT & matrix) {
  typedef typename MatrixT::value_type value_t;
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);

  // Creating local copy for output to prevent interleaving with log
  // messages:
  value_t * matrix_copy = new value_t[matrix.size()];
  auto copy_end = std::copy(matrix.begin(),
                            matrix.end(),
                            matrix_copy);
  DASH_ASSERT(copy_end == matrix_copy + matrix.size());
  cout << "print matrix with copy in C array:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c) {
      cout << " " << setw(5) << matrix_copy[r * cols + c];
    }
    cout << endl;
  }
  delete[] matrix_copy;
}

template<class MatrixT>
void print_matrix_2(const MatrixT & matrix) {
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);

  cout << "print matrix with individual [][] accesses:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c) {
      cout << " " << setw(5) << matrix[r][c];
    }
    cout << endl;
  }
}

template<class MatrixT>
void print_matrix_3(const MatrixT & matrix) {
  typedef typename MatrixT::value_type value_t;
  auto cols = matrix.extent(1);

  cout << "print with global iterator:" << endl;
  uint num= 0;
  for (const auto i : matrix) {

    cout << " " << setw(5) <<  (value_t) i ;
    if ( ++num >= cols ) {
      num= 0;
      cout << endl;
    }
  }
}

template<class MatrixT>
void print_matrix_4(const MatrixT & matrix) {
  auto cols = matrix.extent(1);

  cout << "print _L_ocal/_R_emote with global iterator:" << endl;
  uint num= 0;
  for (const auto &i : matrix) {

    cout << " " << setw(5) << ( i.is_local() ? "L" : "R" ) ;
    if ( ++num >= cols ) {
      num= 0;
      cout << endl;
    }
  }
}

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 2;
  size_t tilesize_y  = 3;
  size_t rows = tilesize_x * num_units * 2;
  size_t cols = tilesize_y * num_units * 2;
  dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(tilesize_x),
                           dash::TILE(tilesize_y)));
  size_t matrix_size = rows * cols;
  DASH_ASSERT(matrix_size == matrix.size());
  DASH_ASSERT(rows == matrix.extent(0));
  DASH_ASSERT(cols == matrix.extent(1));

  if (0 == myid) {
    cout << "Matrix size: " << rows
      << " x " << cols
      << " == " << matrix_size
      << endl;
  }

  // Fill matrix
  if (0 == myid) {
    cout << "== Assigning matrix values as row *1000 + column ==" << endl;
    for(size_t i = 0; i < matrix.extent(0); ++i) {
      for(size_t k = 0; k < matrix.extent(1); ++k) {
        matrix[i][k] = i * 1000 + k;
      }
    }
  }

  // Units waiting for value initialization
  dash::Team::All().barrier();

  // print matrix
  if (0 == myid) {
    print_matrix_1(matrix);
    print_matrix_2(matrix);
    print_matrix_3(matrix);
    print_matrix_4(matrix);
  }

  dash::Team::All().barrier();

  if (0 == myid) {
    cout << "== Assigning matrix values as local unit id ==" << endl;
}
  std::fill(matrix.lbegin(), matrix.lend(), myid);

  dash::Team::All().barrier();

  // print matrix
  if (0 == myid) {
    print_matrix_1(matrix);
    print_matrix_2(matrix);
    print_matrix_3(matrix);
    print_matrix_4(matrix);
  }

  dash::Team::All().barrier();

  dash::finalize();
}

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <iomanip>

#include <libdash.h>

using std::cout;
using std::endl;
using std::setw;

template<class MatrixT>
void print_matrix(const MatrixT & matrix)
{
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
  cout << "Matrix:" << endl;
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c) {
      cout << " " << setw(5) << matrix_copy[r * cols + c];
    }
    cout << endl;
  }
  delete[] matrix_copy;
}

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  size_t myid = dash::myid();
  size_t team_size = dash::Team::All().size();
  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  uint32_t w = 8000;
  uint32_t h = 8000;

  dash::Matrix<uint32_t, 2> matrix(
    dash::SizeSpec<2>( w, h ),
    dash::DistributionSpec<2>( dash::BLOCKED, dash::BLOCKED ),
    dash::Team::All(), teamspec_2d );

  if (0 == myid) {
    for (size_t i = 0; i < w; ++i) {
      for (size_t k = 0; k < h; ++k) {
        matrix[i][k] = 100;
      }
    }
  }

  dash::finalize();
}

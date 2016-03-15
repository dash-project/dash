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
  auto rows = matrix.extent(0);
  auto cols = matrix.extent(1);

  // Creating local copy for output to prevent interleaving with log
  // messages:
  value_t * matrix_copy = new value_t[matrix.size()];
  auto copy_end = std::copy(matrix.begin(),
                            matrix.end(),
                            matrix_copy);
  DASH_ASSERT(copy_end == matrix_copy + matrix.size());
  for (auto r = 0; r < rows; ++r) {
    for (auto c = 0; c < cols; ++c) {
      cout << " " << setw(5) << matrix_copy[r * cols + c];
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
  size_t tilesize_y  = 2;
  size_t rows = tilesize_x * num_units * 2;
  size_t cols = tilesize_y * num_units * 2;
  size_t matrix_size = rows * cols;


    if ( 6 != num_units ) {

        cout << "run me with 6 units" << endl;
        dash::finalize();
        return 1;
    }

  if (0 == myid) {
    cout << "Matrix size: " << rows
       << " x " << cols
       << " == " << matrix_size
       << endl ;
  }

  size_t team_size    = dash::Team::All().size();
  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  if (0 == myid) {
    cout << "    default TeamSpec<2>():" << teamspec_2d.num_units(0) << " x " << teamspec_2d.num_units(1) << endl;
  }
  teamspec_2d.balance_extents();
  if (0 == myid) {
    cout << "    balanced TeamSpec<2>():" << teamspec_2d.num_units(0) << " x " << teamspec_2d.num_units(1) << endl;
  }

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , TILE(4) ) TeamSpec<2>() default" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::TILE(4)),
                         dash::Team::All(),
                         dash::TeamSpec<2>());
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , TILE(4) ) TeamSpec<2>( 1 , num_units )" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::TILE(4)),
                         dash::Team::All(),
                         dash::TeamSpec<2>(1,num_units));
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , TILE(4) ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::TILE(4)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , BLOCKED ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::BLOCKED),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( BLOCKED , TILE(4) ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::BLOCKED,
                           dash::TILE(4)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , CYCLIC ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::CYCLIC),
                           dash::Team::All(),
                           teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( CYCLIC , TILE(4) ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::CYCLIC,
                           dash::TILE(4)),
                           dash::Team::All(),
                           dash::TeamSpec<2>(2,3));
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , BLOCKCYCLIC(4) ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::BLOCKCYCLIC(4)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( BLOCKCYCLIC(4) , TILE(4) ) TeamSpec<2> balanced" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::BLOCKCYCLIC(4),
                           dash::TILE(4)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( BLOCKED , BLOCKED ) is not allowed" << endl;
    }
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( BLOCKCYCLIC(2) , BLOCKCYCLIC(2) ) is not allowed" << endl;
    }
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( CYCLIC , CYCLIC ) is not allowed" << endl;
    }
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , NONE ) TeamSpec<2>( num_units , 1 )" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::NONE),
                         dash::Team::All(),
                         dash::TeamSpec<2>(num_units,1));
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( NONE , TILE(4) ) TeamSpec<2>( 1 , num_units )" << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::NONE,
                           dash::TILE(4)),
                         dash::Team::All(),
                         dash::TeamSpec<2>(1,num_units));
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */

  { /* extra scope for every example */
    if (0 == myid) {
      cout << endl << "Matrix 2D DistributionSpec<2>( TILE(4) , TILE(4) ) TeamSpec<2>() balanced " << endl;
    }

    dash::Matrix<int, 2> matrix(
                         dash::SizeSpec<2>(
                           rows,
                           cols),
                         dash::DistributionSpec<2>(
                           dash::TILE(4),
                           dash::TILE(4)),
                         dash::Team::All(),
                         teamspec_2d);
    DASH_ASSERT(matrix_size == matrix.size());
    DASH_ASSERT(rows == matrix.extent(0));
    DASH_ASSERT(cols == matrix.extent(1));

    dash::Team::All().barrier();

    std::fill(matrix.lbegin(), matrix.lend(), myid);

    dash::Team::All().barrier();

    if (0 == myid) {
      print_matrix(matrix);
    }
    dash::Team::All().barrier();
  } /* extra scope for every example */


  dash::finalize();
}

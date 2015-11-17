/*
 * hello/main.cpp
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <cassert>
#include <iomanip>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[]) {

  pid_t pid;

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
                           dash::TILE(tilesize_x*2),
                           dash::TILE(tilesize_y*2)));
  size_t matrix_size = extent_cols * extent_rows;
  assert(matrix_size == matrix.size());
  assert(extent_cols == matrix.extent(0));
  assert(extent_rows == matrix.extent(1));
  cout << "Matrix size: " << extent_cols << " x " << extent_rows << " == " <<matrix_size << endl;
  // Fill matrix
  if( 0 == myid ) {
    cout << "Assigning matrix values" << endl;
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        matrix[i][k] = (i * 11) + (k * 97);
      }
    }
  }

  // Units waiting for value initialization
  dash::Team::All().barrier();

  // Read and assert values in matrix
  for(int i = 0; i < matrix.extent(0); ++i) {
    for(int k = 0; k < matrix.extent(1); ++k) {
      int value    = matrix[i][k];
      int expected = (i * 11) + (k * 97);
      assert(expected == value);
    }
  }

  // print matrix
  if( 0 == myid ) {
    cout << "Show" << endl;
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        cout << " " << setw(5) << matrix[i][k];
      }
      cout << endl;
    }
  }

  dash::Team::All().barrier();
  matrix[2][2+myid]= 42;

  std::fill( matrix.lbegin(), matrix.lend(), myid );

  dash::Team::All().barrier();

  matrix[2][2+myid]= 42;

  dash::Team::All().barrier();

  // print matrix
  if( 0 == myid ) {
    cout << "Show" << endl;
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        cout << " " << setw(5) << matrix[i][k];
      }
      cout << endl;
    }
  }

  dash::Team::All().barrier();

  if( 0 == myid ) {

    auto mixedblock= matrix.rows(7,4).cols(0,9);
    std::fill( mixedblock.begin(), mixedblock.end(), 8888 );

    auto localblock= matrix.rows(4,4).cols(12,6);
    std::fill( localblock.begin(), localblock.end(), 1111 );

    auto fullblock= matrix.rows(1,2).cols(14,3);
    std::fill( fullblock.begin(), fullblock.end(), 8888 );

    // async alternative ... tbd
    // AsyncMatrixRef<int,2,1> a_col_1 = matrix.async.col(1);
  }

  dash::Team::All().barrier();

  // print matrix
  if( 0 == myid ) {
    cout << "Show" << endl;
    for(int i = 0; i < matrix.extent(0); ++i) {
      for(int k = 0; k < matrix.extent(1); ++k) {
        cout << " " << setw(5) << matrix[i][k];
      }
      cout << endl;
    }
  }

  dash::finalize();
}

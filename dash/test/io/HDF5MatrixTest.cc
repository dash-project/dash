#ifdef DASH_ENABLE_HDF5

#include "HDF5MatrixTest.h"

#include <dash/io/HDF5.h>

#include <dash/Matrix.h>
#include <dash/Dimensional.h>
#include <dash/TeamSpec.h>

#include <dash/algorithm/ForEach.h>
#include <dash/algorithm/SUMMA.h>

#include <dash/pattern/TilePattern.h>
#include <dash/pattern/MakePattern.h>

#include <array>
#include <limits.h>

typedef int value_t;
namespace dio = dash::io::hdf5;

/**
 * Cantors pairing function to map n-tuple to single number
 */
template <typename T, size_t ndim>
T cantorpi(std::array<T, ndim> tuple) {
  int cantor = 0;
  for (int i = 0; i < ndim - 1; i++) {
    T x = tuple[i];
    T y = tuple[i + 1];
    cantor += y + 0.5 * (x + y) * (x + y + 1);
  }
  return cantor;
}

/**
 * Fill a n-dimensional dash matrix with a signatures that contains
 * the global coordinates and a secret which can be the unit id
 * for example
 */
template <typename T, int ndim, typename IndexT, typename PatternT>
void fill_matrix(dash::Matrix<T, ndim, IndexT, PatternT> &matrix,
                 T secret = 0) {
  std::function<void(const T &, IndexT)> f = [&matrix, &secret](T el,
                                                                IndexT i) {
    auto coords = matrix.pattern().coords(i);
    // hack
    *(matrix.begin() + i) = cantorpi(coords) + secret;
  };
  dash::for_each_with_index(matrix.begin(), matrix.end(), f);
}

/**
 * Counterpart to fill_matrix which checks if the given matrix satisfies
 * the desired signature
 */
template <typename T, int ndim, typename IndexT, typename PatternT>
void verify_matrix(dash::Matrix<T, ndim, IndexT, PatternT> &matrix,
                   T secret = 0) {
  std::function<void(const T &, IndexT)> f = [&matrix, &secret](T el,
                                                                IndexT i) {
    auto coords = matrix.pattern().coords(i);
    auto desired = cantorpi(coords) + secret;
    ASSERT_EQ_U(desired, el);
  };
  dash::for_each_with_index(matrix.begin(), matrix.end(), f);
}

TEST_F(HDF5MatrixTest, StoreMultiDimMatrix) {
  typedef dash::TilePattern<2> pattern_t;
  typedef dash::Matrix<value_t, 2, typename pattern_t::index_type, pattern_t>
      matrix_t;

  auto numunits = dash::Team::All().size();
  dash::TeamSpec<2> team_spec(numunits, 1);
  team_spec.balance_extents();

  auto team_extent_x = team_spec.extent(0);
  auto team_extent_y = team_spec.extent(1);

  auto extend_x = 2 * 2 * team_extent_x;
  auto extend_y = 2 * 5 * team_extent_y;

  pattern_t pattern(dash::SizeSpec<2>(extend_x, extend_y),
                    dash::DistributionSpec<2>(dash::TILE(2), dash::TILE(5)),
                    team_spec);

  DASH_LOG_DEBUG("Pattern", pattern);

  int myid = dash::myid();
  {
    matrix_t mat1(pattern);
    dash::barrier();
    LOG_MESSAGE("Matrix created");

    // Fill Array
    for (int x = 0; x < pattern.local_extent(0); x++) {
      for (int y = 0; y < pattern.local_extent(1); y++) {
        mat1.local[x][y] = myid;
      }
    }
    dash::barrier();
    DASH_LOG_DEBUG("BEGIN STORE HDF");

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << mat1;

    DASH_LOG_DEBUG("END STORE HDF");
    dash::barrier();
  }
  dash::barrier();
}

TEST_F(HDF5MatrixTest, StoreSUMMAMatrix) {
  auto myid = dash::myid();
  auto num_units = dash::Team::All().size();
  auto extent_cols = num_units;
  auto extent_rows = num_units;
  auto team_size_x = num_units;
  auto team_size_y = 1;

  // Adopted from SUMMA test case
  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec(team_size_x, team_size_y);
  team_spec.balance_extents();

  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern =
      dash::make_pattern<dash::summa_pattern_partitioning_constraints,
                         dash::summa_pattern_mapping_constraints,
                         dash::summa_pattern_layout_constraints>(size_spec,
                                                                 team_spec);
  DASH_LOG_DEBUG("Pattern", pattern);

  typedef double value_t;
  typedef decltype(pattern) pattern_t;
  typedef typename pattern_t::index_type index_t;

  typedef dash::Matrix<value_t, 2, index_t, pattern_t> matrix_t;

  {
    LOG_MESSAGE("instantiate matrix");
    matrix_t matrix_a(pattern);
    LOG_MESSAGE("matrix instantiated");
    dash::barrier();

    DASH_LOG_DEBUG("fill matrix");
    fill_matrix(matrix_a, static_cast<double>(myid));
    DASH_LOG_DEBUG("matrix filled");
    dash::barrier();

    // Store Matrix
    DASH_LOG_DEBUG("store matrix");
    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
    DASH_LOG_DEBUG("matrix stored");
    dash::barrier();
  }

  matrix_t matrix_b;

  DASH_LOG_DEBUG("restore matrix");
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;
  DASH_LOG_DEBUG("matrix restored");

  dash::barrier();
  DASH_LOG_DEBUG("verify matrix");
  verify_matrix(matrix_b, static_cast<double>(myid));
  DASH_LOG_DEBUG("matrix verified");
}

TEST_F(HDF5MatrixTest, AutoGeneratePattern) {
  {
    dash::Matrix<int, 2> matrix_a(
                           dash::SizeSpec<2>(
                             dash::size(),
                             dash::size()));
    // Fill
    fill_matrix(matrix_a);
    dash::barrier();

    dio::OutputStream os(_filename);
    os << dio::store_pattern(false) << dio::dataset(_dataset) << matrix_a;

    dash::barrier();
  }
  dash::Matrix<int, 2> matrix_b;

  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  dash::barrier();

  // Verify
  verify_matrix(matrix_b);
}

/**
 * Import data into a already allocated matrix
 * because matrix_a and matrix_b are allocated the same way
 * it is expected that each unit remains its local ranges
 */
TEST_F(HDF5MatrixTest, PreAllocation) {
  int ext_x = dash::size();
  int ext_y = ext_x * 2 + 1;
  {
    dash::Matrix<int, 2> matrix_a(
                           dash::SizeSpec<2>(
                             ext_x,
                             ext_y));
    // Fill
    fill_matrix(matrix_a, static_cast<int>(dash::myid()));
    dash::barrier();

    dio::OutputStream os(_filename);
    os << dio::store_pattern(false) << dio::dataset(_dataset) << matrix_a;

    dash::barrier();
  }
  dash::Matrix<int, 2> matrix_b(dash::SizeSpec<2>(ext_x, ext_y));

  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  dash::barrier();

  // Verify
  verify_matrix(matrix_b, static_cast<int>(dash::myid()));
}

/**
 * Allocate a matrix with extents that cannot fit into full blocks
 */
TEST_F(HDF5MatrixTest, UnderfilledPattern) {
  typedef dash::Pattern<2, dash::ROW_MAJOR> pattern_t;
  typedef typename pattern_t::index_type index_t;

  size_t team_size = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto block_size_x = 12;
  auto block_size_y = 4;
  int  ext_x = (block_size_x * teamspec_2d.num_units(0)) - 3;
  int  ext_y = (block_size_y * teamspec_2d.num_units(1)) - 1;

  LOG_MESSAGE("Matrix extent (%i,%i)", ext_x, ext_y);

  auto size_spec = dash::SizeSpec<2>(ext_x, ext_y);

  // Check TilePattern
  const pattern_t pattern(size_spec,
                          dash::DistributionSpec<2>(dash::TILE(block_size_x),
                                                    dash::TILE(block_size_y)),
                          teamspec_2d, dash::Team::All());

  {
    dash::Matrix<int, 2, index_t, pattern_t> matrix_a;
    matrix_a.allocate(pattern);

    fill_matrix(matrix_a, 1);

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
  }
  dash::barrier();

  dash::Matrix<int, 2, index_t, pattern_t> matrix_b;
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  verify_matrix(matrix_b, 1);
}

template<typename MatrixT>
void print_matrix(const MatrixT& matrix) {
  using namespace std;
  dash::barrier();
  if(!dash::myid()) {
    auto rows = matrix.extent(0);
    auto cols = matrix.extent(1);
    cout << fixed;
    cout.precision(4);
    cout << "Matrix:" << endl;
    for (auto r = 0; r < rows; ++r) {
      for (auto c = 0; c < cols; ++c) {
        auto lindex = matrix.pattern().local_index({r,c});
        cout << " " << setw(3) << (double) matrix[r][c] << "(" << lindex.unit << "," << lindex.index << ")";
      }
      cout << endl;
    }
  }
  dash::barrier();
}

/**
 * Allocate a matrix with extents that cannot fit into full blocks
 * TilePattern
 */
TEST_F(HDF5MatrixTest, UnderfilledPatternTile) {
  using Matrix_t = dash::Matrix<double, 2>;

  auto team_size = dash::Team::All().size();

  int ext_x = 5 * team_size + 1;
  int ext_y = 10 * team_size;

  double test_value = dash::myid()+1;

  LOG_MESSAGE("Matrix extent (%i,%i)", ext_x, ext_y);

  auto size_spec = dash::SizeSpec<2>(ext_x, ext_y);

  {
    Matrix_t matrix_a(size_spec);
    //dash::fill(matrix_a.begin(), matrix_a.end(), test_value);
    fill_matrix(matrix_a, test_value);
    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
  }

  Matrix_t matrix_b(size_spec);
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;
  dash::barrier();

  verify_matrix(matrix_b, test_value);
}

TEST_F(HDF5MatrixTest, UnderfilledPatMultiple) {
  typedef dash::Pattern<2, dash::ROW_MAJOR> pattern_t;
  typedef typename pattern_t::index_type index_t;

  size_t team_size = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto block_size_x = 12;
  auto block_size_y = 4;
  auto ext_x = (block_size_x * (teamspec_2d.num_units(0) + 1)) - 3;
  auto ext_y = (block_size_y * (teamspec_2d.num_units(1) + 1)) - 1;

  auto size_spec = dash::SizeSpec<2>(ext_x, ext_y);

  // Create BlockPattern
  const pattern_t pattern(size_spec,
                          dash::DistributionSpec<2>(dash::TILE(block_size_x),
                                                    dash::TILE(block_size_y)),
                          teamspec_2d, dash::Team::All());

  {
    dash::Matrix<int, 2, index_t, pattern_t> matrix_a;
    matrix_a.allocate(pattern);

    fill_matrix(matrix_a);

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
  }
  dash::barrier();

  // restore to simpler pattern
  dash::Matrix<int, 2, index_t, pattern_t> matrix_b(ext_x, ext_y);
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  verify_matrix(matrix_b);
}

TEST_F(HDF5MatrixTest, UnderfilledMultDim) {
  typedef dash::Pattern<3, dash::ROW_MAJOR> pattern_t;
  typedef typename pattern_t::index_type index_t;
  typedef unsigned long size_type;

  size_t team_size = dash::Team::All().size();

  dash::TeamSpec<3> teamspec_3d(team_size, 1, 1);
  teamspec_3d.balance_extents();

  std::array<size_type, 3> block_size{{2, 3, 4}};
  std::array<size_type, 3> extents = {};
  for (int i = 0; i < extents.size(); ++i) {
    extents[i] = (block_size[i] * teamspec_3d.num_units(i) + 1) - i;
  }

  auto size_spec = dash::SizeSpec<3, size_type>(extents);

  // Create BlockPattern
  const pattern_t pattern(size_spec,
                          dash::DistributionSpec<3>(dash::TILE(block_size[0]),
                                                    dash::TILE(block_size[1]),
                                                    dash::TILE(block_size[2])),
                          teamspec_3d, dash::Team::All());

  {
    dash::Matrix<int, 3, index_t, pattern_t> matrix_a;
    matrix_a.allocate(pattern);

    fill_matrix(matrix_a);

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
  }
  dash::barrier();

  // restore to simpler pattern
  dash::Matrix<int, 3, index_t, pattern_t> matrix_b(size_spec);
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  verify_matrix(matrix_b);
}

TEST_F(HDF5MatrixTest, UnderfilledPatMultipleBlocks) {
  typedef dash::Pattern<2, dash::ROW_MAJOR> pattern_t;
  typedef typename pattern_t::index_type index_t;

  size_t team_size = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto block_size_x = 3;
  auto block_size_y = 4;
  // 6 fully filled blocks + 1 underfilled ( 1 missing )
  auto ext_x = (block_size_x * (teamspec_2d.num_units(0) * 2 + 1)) + 2;
  // 5 fully filled blocks + 1 underfilled ( 1 missing )
  auto ext_y = (block_size_y * ((teamspec_2d.num_units(1) * 3))) + 3;

  auto size_spec = dash::SizeSpec<2>(ext_x, ext_y);

  // Create BlockPattern
  const pattern_t pattern(size_spec,
                          dash::DistributionSpec<2>(dash::TILE(block_size_x),
                                                    dash::TILE(block_size_y)),
                          teamspec_2d, dash::Team::All());

  {
    dash::Matrix<int, 2, index_t, pattern_t> matrix_a;
    matrix_a.allocate(pattern);

    fill_matrix(matrix_a);

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a;
  }
  dash::barrier();

  // restore to simpler pattern
  dash::Matrix<int, 2, index_t, pattern_t> matrix_b(ext_x, ext_y);
  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_b;

  verify_matrix(matrix_b);
}

TEST_F(HDF5MatrixTest, MultipleDatasets) {
  int ext_x = dash::size() * 5;
  int ext_y = dash::size() * 3;
  int secret_a = 10;
  double secret_b = 3;

  {
    dash::Matrix<int,    2> matrix_a(dash::SizeSpec<2>(ext_x,ext_y));
    dash::Matrix<double, 2> matrix_b(dash::SizeSpec<2>(ext_x,ext_y));

    // Fill
    fill_matrix(matrix_a, secret_a);
    fill_matrix(matrix_b, secret_b);
    dash::barrier();

    dio::OutputStream os(_filename);
    os << dio::dataset(_dataset) << matrix_a << dio::dataset("datasettwo")
       << matrix_b;
    dash::barrier();
  }

  dash::Matrix<int, 2> matrix_c;
  dash::Matrix<double, 2> matrix_d;

  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_c >> dio::dataset("datasettwo") >>
      matrix_d;

  dash::barrier();

  // Verify data
  verify_matrix(matrix_c, secret_a);
  verify_matrix(matrix_d, secret_b);
}

TEST_F(HDF5MatrixTest, ModifyDataset) {
  int ext_x = dash::size() * 5;
  int ext_y = dash::size() * 3;
  double secret_a = 10;
  double secret_b = 3;
  {
    dash::Matrix<double,2> matrix_a(dash::SizeSpec<2>(ext_x,ext_y));
    dash::Matrix<double,2> matrix_b(dash::SizeSpec<2>(ext_x,ext_y));

    // Fill
    fill_matrix(matrix_a, secret_a);
    fill_matrix(matrix_b, secret_b);
    dash::barrier();

    {
      dio::OutputStream os(_filename);
      os << dio::dataset(_dataset) << matrix_a;
    }

    dash::barrier();
    // overwrite first data
    dio::OutputStream os(_filename, dio::DeviceMode::app);
    os << dio::dataset(_dataset) << dio::modify_dataset() << matrix_b;
    dash::barrier();
  }
  dash::Matrix<double, 2> matrix_c;

  dio::InputStream is(_filename);
  is >> dio::dataset(_dataset) >> matrix_c;

  dash::barrier();

  // Verify data
  verify_matrix(matrix_c, secret_b);
}

TEST_F(HDF5MatrixTest, GroupTest) {
  int ext_x = dash::size() * 5;
  int ext_y = dash::size() * 2;
  double secret[] = {10, 11, 12};
  {
    dash::Matrix<double, 2> matrix_a(ext_x, ext_y);
    dash::Matrix<double, 2> matrix_b(ext_x, ext_y);
    dash::Matrix<double, 2> matrix_c(ext_x, ext_y);

    // Fill
    fill_matrix(matrix_a, secret[0]);
    fill_matrix(matrix_b, secret[1]);
    fill_matrix(matrix_c, secret[2]);
    dash::barrier();

    // Set option
    dio::OutputStream os(_filename);
    os << dio::dataset("matrix_a") << matrix_a << dio::dataset("g1/matrix_b")
       << matrix_b << dio::dataset("g1/g2/matrix_c") << matrix_c;

    dash::barrier();
  }
  dash::Matrix<double, 2> matrix_a;
  dash::Matrix<double, 2> matrix_b;
  dash::Matrix<double, 2> matrix_c;
  dio::InputStream is(_filename);
  is >> dio::dataset("matrix_a") >> matrix_a >> dio::dataset("g1/matrix_b") >>
      matrix_b >> dio::dataset("g1/g2/matrix_c") >> matrix_c;

  dash::barrier();

  // Verify data
  verify_matrix(matrix_a, secret[0]);
  verify_matrix(matrix_b, secret[1]);
  verify_matrix(matrix_c, secret[2]);
}

#if 0
TEST_F(HDF5MatrixTest, DashView)
{
  int secret = 0;
  dash::NArray<int,2> matrix2d(100,80);
  fill_matrix(matrix2d, secret);

  {
    auto output_view = dash::sub(0,5, matrix2d);

    // TODO: Does not work
    dash::fill(dash::begin(output_view), dash::end(output_view), 5);

    // Store View
    dio::OutputStream os(_filename);
    os << output_view;
  }

  // Load data into a existing view
  auto input_view = dash::sub(0,10, matrix2d);
  dio::InputStream is(_filename);
  is >> input_view;

  // Load data into new matrix
  dash::Array<int> matrix1d;
  is >> matrix1d;
}
#endif

#if 0  // TODO: Implement and re-enable
TEST_F(HDF5MatrixTest, HDFView)
{
  int secret = 0;
  dash::NArray<int,3> matrix3d(10,8,5);
  fill_matrix(matrix3d, secret);

  {
    dio::OutputStream os(_filename);
    os << matrix3d;
  }

  // Load subset of data into new matrix
  dash::NArray<int,3> sub_matrix3d;
  auto position = std::array<size_t,3>({2,4,1}); // top left corner of desired block
  auto extent   = std::array<size_t,3>({4,2,1}); // extents of block
  dio::InputStream is(_filename);
  is >> dash::block(position, extent)
     >> sub_matrix3d;

  // post condition
  // sub_matrix3d has extent {4,2,1}
}

TEST_F(HDF5MatrixTest, WriteCorresponding)
{
  int secret = 0;
  dash::NArray<int,3> matrix3d(10,8,5);
  fill_matrix(matrix3d, secret);

  {
    dio::OutputStream os(_filename);
    os << matrix3d;
  }

  auto view = matrix3d.col(1);
  // modify view
  dash::fill(view.begin(), view.end(), 0);

  // write slice back
  dio::OutputStream os(_filename, dio::DeviceMode::App);
  is >> dio::modify_dataset()
     >> dash::block(position, extent)
     >> sub_matrix3d;

  // post condition
  // slice in file is updated
  dash::NArray<int,3> new_matrix3d(10,8,5);

  dio::InputStream is(_filename);
  is << new_matrix3d;

  auto view = new_matrix3d.col(1);
  dash::for_each(view.begin(), view.end(), [](int & el){ASSERT_EQ_U(0, el)});
}

#endif
#if 0
// Test Conversion between dash::Array and dash::Matrix
// Currently not possible as matrix has to be at least
// two dimensional
TEST_F(HDF5MatrixTest, ArrayToMatrix)
{
  {
    auto array = dash::Array<int>(100, dash::CYCLIC);
    if (dash::myid() == 0) {
      for (int i = 0; i < array.size(); i++) {
        array[i] = i;
      }
    }

    // Do not store pattern
    auto fopts = dio::StoreHDF::hdf5_options();
    fopts.store_pattern = false;

    dio::StoreHDF::write(array, _filename, _dataset);
    dash::barrier();
  }
  dash::Matrix<int, 1> matrix;
  dio::StoreHDF::read(matrix, _filename, _dataset);
  dash::barrier();

  // Verify
  if (dash::myid() == 0) {
    for (int i = 0; i < matrix.extent(0); i++) {
      ASSERT_EQ(i, matrix[i]);
    }
  }
}
#endif

#endif  // DASH_ENABLE_HDF5

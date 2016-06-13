#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5MatrixTest.h"

#include "limits.h"

typedef int value_t;
namespace dio = dash::io;

/**
 * Cantors pairing function to map n-tuple to single number
 */
template <
  typename T,
  size_t ndim >
T cantorpi(std::array<T, ndim> tuple)
{
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
template <
  typename T,
  int ndim,
  typename IndexT,
  typename PatternT >
void fill_matrix(dash::Matrix<T, ndim, IndexT, PatternT> & matrix,
                 T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&matrix, &secret](T el, IndexT i) {
    auto coords = matrix.pattern().coords(i);
    // hack
    *(matrix.begin() + i) = cantorpi(coords) + secret;
  };
  dash::for_each_with_index(
    matrix.begin(),
    matrix.end(),
    f);
}

/**
 * Counterpart to fill_matrix which checks if the given matrix satisfies
 * the desired signature
 */
template <
  typename T,
  int ndim,
  typename IndexT,
  typename PatternT >
void verify_matrix(dash::Matrix<T, ndim, IndexT, PatternT> & matrix,
                   T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&matrix, &secret](T el, IndexT i) {
    auto coords  = matrix.pattern().coords(i);
    auto desired = cantorpi(coords) + secret;
    ASSERT_EQ_U(
      desired,
      el);
  };
  dash::for_each_with_index(
    matrix.begin(),
    matrix.end(),
    f);
}


TEST_F(HDF5MatrixTest, StoreMultiDimMatrix)
{
  typedef dash::TilePattern<2>  pattern_t;
  typedef dash::Matrix <
  value_t,
  2,
  long,
  pattern_t >          matrix_t;

  auto numunits =  dash::Team::All().size();
  dash::TeamSpec<2> team_spec(numunits, 1);
  team_spec.balance_extents();

  auto team_extent_x = team_spec.extent(0);
  auto team_extent_y = team_spec.extent(1);

  auto extend_x = 2 * 2 * team_extent_x;
  auto extend_y = 2 * 5 * team_extent_y;

  pattern_t pattern(
    dash::SizeSpec<2>(
      extend_x,
      extend_y),
    dash::DistributionSpec<2>(
      dash::TILE(2),
      dash::TILE(5)),
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

		dio::HDF5OutputStream os(_filename);
    os << dio::HDF5dataset(_dataset) << mat1;

    DASH_LOG_DEBUG("END STORE HDF");
    dash::barrier();
  }
  dash::barrier();
}

TEST_F(HDF5MatrixTest, StoreSUMMAMatrix)
{
  typedef double  value_t;
  typedef long    index_t;

  auto myid         = dash::myid();
  auto num_units   = dash::Team::All().size();
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
  auto pattern = dash::make_pattern <
                 dash::summa_pattern_partitioning_constraints,
                 dash::summa_pattern_mapping_constraints,
                 dash::summa_pattern_layout_constraints > (
                   size_spec,
                   team_spec);
  DASH_LOG_DEBUG("Pattern", pattern);

  {
    // Instantiate Matrix
    dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_a(pattern);
    dash::barrier();

    fill_matrix(matrix_a, static_cast<double>(myid));
    dash::barrier();

    // Store Matrix
		dio::HDF5OutputStream os(_filename);
    os << dio::HDF5dataset(_dataset) << matrix_a;

    dash::barrier();
  }

  dash::Matrix<double, 2> matrix_b;

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset) >> matrix_b;

  dash::barrier();
  verify_matrix(matrix_b, static_cast<double>(myid));
}

TEST_F(HDF5MatrixTest, AutoGeneratePattern)
{
  {
    auto matrix_a = dash::Matrix<int, 2>(
                      dash::SizeSpec<2>(
                        dash::size(),
                        dash::size()));
    // Fill
    fill_matrix(matrix_a);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5store_pattern(false)
       << dio::HDF5dataset(_dataset)
			 << matrix_a;

    dash::barrier();
  }
  dash::Matrix<int, 2> matrix_b;

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset) >> matrix_b;

  dash::barrier();

  // Verify
  verify_matrix(matrix_b);
}

// Import data into a already allocated matrix
// because matrix_a and matrix_b are allocated the same way
// it is expected that each unit remains its local ranges
TEST_F(HDF5MatrixTest, PreAllocation)
{
  int ext_x = dash::size();
  int ext_y = ext_x * 2 + 1;
  {
    auto matrix_a = dash::Matrix<int, 2>(
                      dash::SizeSpec<2>(
                        ext_x,
                        ext_y));
    // Fill
    fill_matrix(matrix_a, dash::myid());
    dash::barrier();

    // Set option
		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5store_pattern(false)
       << dio::HDF5dataset(_dataset)
			 << matrix_a;

    dash::barrier();
  }
  dash::Matrix<int, 2> matrix_b(
    dash::SizeSpec<2>(
      ext_x,
      ext_y));

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset) >> matrix_b;

  dash::barrier();

  // Verify
  verify_matrix(matrix_b, dash::myid());
}

/**
 * Allocate a matrix with extents that cannot fit into full blocks
 */
#if 1
TEST_F(HDF5MatrixTest, UnderfilledPattern)
{
  typedef dash::Pattern<2, dash::ROW_MAJOR> pattern_t;

  size_t team_size    = dash::Team::All().size();

  dash::TeamSpec<2> teamspec_2d(team_size, 1);
  teamspec_2d.balance_extents();

  auto block_size_x = 10;
  auto block_size_y = 15;
  auto ext_x        = (block_size_x * teamspec_2d.num_units(0)) - 3;
  auto ext_y        = (block_size_y * teamspec_2d.num_units(1)) - 1;

  auto size_spec = dash::SizeSpec<2>(ext_x, ext_y);

  // Check TilePattern
  const pattern_t pattern(
    size_spec,
    dash::DistributionSpec<2>(
      dash::TILE(block_size_x),
      dash::TILE(block_size_y)),
    teamspec_2d,
    dash::Team::All());

  {
    dash::Matrix<int, 2, long, pattern_t> matrix_a;
    matrix_a.allocate(pattern);

    fill_matrix(matrix_a);

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << matrix_a;		
  }
  dash::barrier();

  dash::Matrix<int, 2, long, pattern_t> matrix_b;
	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset)
     >> matrix_b;
}

TEST_F(HDF5MatrixTest, MultipleDatasets)
{
	int    ext_x    = dash::size() * 5;
	int    ext_y    = dash::size() * 3;
	int    secret_a = 10;
	double secret_b = 3;
	{
    auto matrix_a = dash::Matrix<int,2>(dash::SizeSpec<2>(ext_x,ext_y));
		auto matrix_b = dash::Matrix<double,2>(dash::SizeSpec<2>(ext_x,ext_y));

    // Fill
    fill_matrix(matrix_a, secret_a);
		fill_matrix(matrix_b, secret_b);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.overwrite_file = false;

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << matrix_a
			 << dio::HDF5dataset("datasettwo")
       << matrix_b;
    dash::barrier();
  }
  dash::Matrix<int,2>    matrix_c;
	dash::Matrix<double,2> matrix_d;

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset)
     >> matrix_c
		 >> dio::HDF5dataset("datasettwo")
		 >> matrix_d;
	
  dash::barrier();

  // Verify data
  verify_matrix(matrix_c, secret_a);
 	verify_matrix(matrix_d, secret_b);
}

TEST_F(HDF5MatrixTest, ModifyDataset)
{
	int    ext_x    = dash::size() * 5;
	int    ext_y    = dash::size() * 3;
	double secret_a = 10;
	double secret_b = 3;
	{
    auto matrix_a = dash::Matrix<double,2>(dash::SizeSpec<2>(ext_x,ext_y));
		auto matrix_b = dash::Matrix<double,2>(dash::SizeSpec<2>(ext_x,ext_y));

    // Fill
    fill_matrix(matrix_a, secret_a);
		fill_matrix(matrix_b, secret_b);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.overwrite_file = false;

		{
			dio::HDF5OutputStream os(_filename);
			os << dio::HDF5dataset(_dataset)
    	   << matrix_a;
		}

		dash::barrier();
		// overwrite first data
		dio::HDF5OutputStream os(_filename, dio::HDF5FileOptions::Append);
		os << dio::HDF5dataset(_dataset)
       << dio::HDF5modify_dataset()
       << matrix_b;
    dash::barrier();
  }
  dash::Matrix<double,2>    matrix_c;

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset)
     >> matrix_c;
	
  dash::barrier();

  // Verify data
  verify_matrix(matrix_c, secret_b);
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
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;

    dash::io::StoreHDF::write(array, _filename, _dataset);
    dash::barrier();
  }
  dash::Matrix<int, 1> matrix;
  dash::io::StoreHDF::read(matrix, _filename, _dataset);
  dash::barrier();

  // Verify
  if (dash::myid() == 0) {
    for (int i = 0; i < matrix.extent(0); i++) {
      ASSERT_EQ(i, matrix[i]);
    }
  }
}
#endif
#endif // DASH_ENABLE_HDF5



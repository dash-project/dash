#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5ArrayTest.h"

#include "limits.h"

typedef int value_t;
typedef dash::Array<value_t, long> array_t;

/**
 * Fill an dash array with a signatures that contains
 * the global coordinates and a secret which can be the unit id
 * for example
 */
template <
  typename T,
  typename IndexT,
  typename PatternT >
void fill_array(dash::Array<T, IndexT, PatternT> & array, T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&array, &secret](T el, IndexT i) {
    auto coords = array.pattern().coords(i);
    // hack
    *(array.begin() + i) = coords[0] + secret;
  };
  dash::for_each_with_index(
    array.begin(),
    array.end(),
    f);
}

/**
 * Counterpart to fill_array which checks if the given array satisfies
 * the desired signature
 */
template <
  typename T,
  typename IndexT,
  typename PatternT >
void verify_array(dash::Array<T, IndexT, PatternT> & array,
                  T secret = 0)
{
  std::function< void(const T &, IndexT)>
  f = [&array, &secret](T el, IndexT i) {
    auto coords  = array.pattern().coords(i);
    auto desired = coords[0] + secret;
    ASSERT_EQ_U(
      desired,
      el);
  };
  dash::for_each_with_index(
    array.begin(),
    array.end(),
    f);
}

TEST_F(HDFArrayTest, StoreLargeDashArray)
{
  // Pattern for arr2 array
  size_t nunits           = dash::Team::All().size();
  size_t tilesize         = 512 * 512;
  size_t blocks_per_unit  = 4; //32;
  size_t size             = nunits * tilesize * blocks_per_unit;
  long   mbsize_total     = (size * sizeof(value_t)) / (tilesize);
  long   mbsize_unit      = mbsize_total / nunits;

  // Add some randomness to the data
  std::srand(time(NULL));
  int local_secret = std::rand() % 1000;
  int myid         = dash::myid();

  {
    // Create first array
    array_t arr1(size, dash::TILE(tilesize));

    // Fill Array
    fill_array(arr1, local_secret);

    dash::barrier();

    DASH_LOG_DEBUG("Estimated memory per rank: ", mbsize_unit, "MB");
    DASH_LOG_DEBUG("Estimated memory total: ", mbsize_total, "MB");
    DASH_LOG_DEBUG("Array filled, begin hdf5 store");

    dash::io::StoreHDF::write(arr1, _filename, _table);
    dash::barrier();
  }
  DASH_LOG_DEBUG("Array successfully written ");

  // Create second array
  dash::Array<value_t> arr2;
  dash::barrier();
  dash::io::StoreHDF::read(arr2, _filename, _table);

  dash::barrier();
  verify_array(arr2, local_secret);
}

TEST_F(HDFArrayTest, AutoGeneratePattern)
{
  {
    auto array_a = dash::Array<int>(dash::size() * 2);
    // Fill
    fill_array(array_a);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b;
  dash::io::StoreHDF::read(array_b, _filename, _table);
  dash::barrier();

  // Verify
  verify_array(array_b);
}

// Import data into a already allocated array
// because array_a and array_b are allocated the same way
// it is expected that each unit remains its local ranges
TEST_F(HDFArrayTest, PreAllocation)
{
  int ext_x = dash::size() * 2;
  {
    auto array_a = dash::Array<int>(ext_x);
    // Fill
    fill_array(array_a, dash::myid());
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);
  dash::io::StoreHDF::read(array_b, _filename, _table);
  dash::barrier();

  // Verify
  verify_array(array_b, dash::myid());
}

// Test Stream API
TEST_F(HDFArrayTest, OutputStream)
{
  {
    auto array_a = dash::Array<long>(dash::size() * 2);

    fill_array(array_a);
    dash::barrier();

    auto fopts = dash::io::StoreHDF::get_default_options();
    auto os  = dash::io::HDF5OutputStream(_filename);
    os   << dash::io::HDF5Table(_table)
         << fopts
         << array_a;
  }
  dash::barrier();
  // Import data
  dash::Array<long> array_b;
  auto is = dash::io::HDF5OutputStream(_filename);
  (is << dash::io::HDF5Table(_table)) >> array_b;

  verify_array(array_b);
}

TEST_F(HDFArrayTest, UnderfilledPattern)
{
  int   ext_x = dash::size() * 5 + 1;
  long  tilesize;
  {
    auto array_a = dash::Array<int>(ext_x);
    tilesize = array_a.pattern().blocksize(0);
    // Fill
    fill_array(array_a);
    dash::barrier();
    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    // Important as recreation should create equal pattern
    fopts.store_pattern = true;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b;
  dash::io::StoreHDF::read(array_b, _filename, _table);
  dash::barrier();

  // Verify
  // Check extents
  DASH_ASSERT_EQ(
    ext_x,
    array_b.size(),
    "Array extent does not match input array");
  // Check tilesize
  DASH_ASSERT_EQ(
    tilesize,
    array_b.pattern().blocksize(0),
    "Tilesizes do not match");
  // Verify data
  verify_array(array_b);
}

TEST_F(HDFArrayTest, UnderfilledPatPreAllocate)
{
  int ext_x = dash::size() * 5 + 1;
  {
    auto array_a = dash::Array<int>(ext_x);
    // Fill
    fill_array(array_a);
    dash::barrier();
    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);
  dash::io::StoreHDF::read(array_b, _filename, _table);
  dash::barrier();

  // Verify
  // Check extents
  DASH_ASSERT_EQ(
    ext_x,
    array_b.size(),
    "Array extent does not match input array");
  // Verify data
  verify_array(array_b);
}

TEST_F(HDFArrayTest, MultipleDatasets)
{
	int    ext_x    = dash::size() * 5;
	int    secret_a = 10;
	double secret_b = 3;
	{
    auto array_a = dash::Array<int>(ext_x);
		auto array_b = dash::Array<double>(ext_x*2);

    // Fill
    fill_array(array_a, secret_a);
		fill_array(array_b, secret_b);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.overwrite_file = false;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
		dash::io::StoreHDF::write(array_b, _filename, "tabletwo", fopts);
    dash::barrier();
  }
  dash::Array<int>    array_c;
	dash::Array<double> array_d;
	dash::io::StoreHDF::read(array_c, _filename, _table);
	dash::io::StoreHDF::read(array_d, _filename, "tabletwo");
	
  dash::barrier();

  // Verify data
  verify_array(array_c, secret_a);
 	verify_array(array_d, secret_b);
}

TEST_F(HDFArrayTest, ModifyDataset)
{
	int    ext_x    = dash::size() * 5;
	double secret_a = 10;
	double secret_b = 3;
	{
    auto array_a = dash::Array<double>(ext_x);
		auto array_b = dash::Array<double>(ext_x);

    // Fill
    fill_array(array_a, secret_a);
		fill_array(array_b, secret_b);
    dash::barrier();

    // Set option
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.overwrite_file = false;

    dash::io::StoreHDF::write(array_a, _filename, _table, fopts);
		dash::barrier();
		// overwrite first data
		fopts.modify_dataset = true;
		dash::io::StoreHDF::write(array_b, _filename, _table, fopts);
    dash::barrier();
  }
  dash::Array<double>    array_c;
	dash::io::StoreHDF::read(array_c, _filename, _table);
	
  dash::barrier();

  // Verify data
  verify_array(array_c, secret_b);
}
#endif // DASH_ENABLE_HDF5



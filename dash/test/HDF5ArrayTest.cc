#ifdef DASH_ENABLE_HDF5

#include <libdash.h>

#include <limits.h>
#include <gtest/gtest.h>

#include "HDF5ArrayTest.h"
#include "TestBase.h"
#include "TestLogHelpers.h"


typedef int value_t;
typedef dash::Array<value_t, long> array_t;

namespace dio = dash::io::hdf5;

using dash::io::hdf5::StoreHDF;
using dash::io::hdf5::FileOptions;
using dash::io::hdf5::InputStream;
using dash::io::hdf5::OutputStream;
using dash::io::hdf5::dataset;
using dash::io::hdf5::modify_dataset;
using dash::io::hdf5::store_pattern;
using dash::io::hdf5::restore_pattern;
using dash::io::hdf5::setpattern_key;

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

TEST_F(HDF5ArrayTest, StoreLargeDashArray)
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

    StoreHDF::write(arr1, _filename, _dataset);
    dash::barrier();
  }
  DASH_LOG_DEBUG("Array successfully written ");

  // Create second array
  dash::Array<value_t> arr2;
  dash::barrier();
  StoreHDF::read(arr2, _filename, _dataset);

  dash::barrier();
  verify_array(arr2, local_secret);
}

TEST_F(HDF5ArrayTest, AutoGeneratePattern)
{
  {
    auto array_a = dash::Array<int>(dash::size() * 2);
    // Fill
    fill_array(array_a);
    dash::barrier();

    // Set option
    auto fopts = StoreHDF::get_default_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b;
  StoreHDF::read(array_b, _filename, _dataset);
  dash::barrier();

  // Verify
  verify_array(array_b);
}

// Import data into a already allocated array
// because array_a and array_b are allocated the same way
// it is expected that each unit remains its local ranges
TEST_F(HDF5ArrayTest, PreAllocation)
{
  int ext_x = dash::size() * 2;
  {
    auto array_a = dash::Array<int>(ext_x);
    // Fill
    fill_array(array_a, dash::myid());
    dash::barrier();

    // Set option
    auto fopts = StoreHDF::get_default_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);
  StoreHDF::read(array_b, _filename, _dataset);
  dash::barrier();

  // Verify
  verify_array(array_b, dash::myid());
}

// Test Stream API
TEST_F(HDF5ArrayTest, OutputStreamOpen)
{
  {
    auto array_a = dash::Array<long>(dash::size() * 2);

    fill_array(array_a);
    dash::barrier();

    auto os  = OutputStream(_filename);
    os   << dio::dataset(_dataset)
         << array_a;
  }
  dash::barrier();
  // Import data
  dash::Array<long> array_b;
  auto is = InputStream(_filename);
  is >> dio::dataset(_dataset) >> array_b;

  verify_array(array_b);
}

TEST_F(HDF5ArrayTest, UnderfilledPattern)
{
  int   ext_x = dash::size() * 5 + 1;
  long  tilesize;
  {
    auto array_a = dash::Array<int>(ext_x);
    tilesize     = array_a.pattern().blocksize(0);
    // Fill
    fill_array(array_a);
    dash::barrier();
    // Set option
    auto fopts = StoreHDF::get_default_options();
    // Important as recreation should create equal pattern
    fopts.store_pattern = true;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<int> array_b;
  StoreHDF::read(array_b, _filename, _dataset);
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

TEST_F(HDF5ArrayTest, UnderfilledPatPreAllocate)
{
  int ext_x = dash::size() * 5 + 1;
  {
    auto array_a = dash::Array<int>(ext_x);
    // Fill
    fill_array(array_a);
    dash::barrier();
    // Set option
    auto fopts = StoreHDF::get_default_options();
    fopts.store_pattern = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);
  StoreHDF::read(array_b, _filename, _dataset);
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

TEST_F(HDF5ArrayTest, MultipleDatasets)
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
    auto fopts = StoreHDF::get_default_options();
    fopts.overwrite_file = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
		StoreHDF::write(array_b, _filename, "datasettwo", fopts);
    dash::barrier();
  }
  dash::Array<int>    array_c;
	dash::Array<double> array_d;
	StoreHDF::read(array_c, _filename, _dataset);
	StoreHDF::read(array_d, _filename, "datasettwo");

  dash::barrier();

  // Verify data
  verify_array(array_c, secret_a);
  verify_array(array_d, secret_b);
}

TEST_F(HDF5ArrayTest, ModifyDataset)
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
    auto fopts = StoreHDF::get_default_options();
    fopts.overwrite_file = false;

    StoreHDF::write(array_a, _filename, _dataset, fopts);
		dash::barrier();
		// overwrite first data
		fopts.modify_dataset = true;
		StoreHDF::write(array_b, _filename, _dataset, fopts);
    dash::barrier();
  }
  dash::Array<double>    array_c;
	StoreHDF::read(array_c, _filename, _dataset);

  dash::barrier();

  // Verify data
  verify_array(array_c, secret_b);
}

TEST_F(HDF5ArrayTest, StreamCreationFlags)
{
int    ext_x    = dash::size() * 5;
double secret = 10;
{
  auto array_a = dash::Array<double>(ext_x);

  // Fill
  fill_array(array_a, secret);
  dash::barrier();

  // Set option

  OutputStream os(_filename, FileOptions::Append);
  os << dio::dataset("settwo")
     << dio::setpattern_key("custom_dash_pattern")
     << dio::store_pattern()
     << array_a
     << dio::modify_dataset()
     << array_a;

  dash::barrier();
}
dash::Array<double>    array_b;
InputStream is(_filename);
is >> dio::dataset("settwo")
   >> dio::setpattern_key("custom_dash_pattern")
   >> dio::restore_pattern()
   >> array_b;

dash::barrier();

// Verify data
verify_array(array_b, secret);
}

TEST_F(HDF5ArrayTest, GroupTest)
{
  int    ext_x    = dash::size() * 5;
  double secret[] = {10,11,12};
  {
    auto array_a = dash::Array<double>(ext_x);
    auto array_b = dash::Array<double>(ext_x);
    auto array_c = dash::Array<double>(ext_x);
  
    // Fill
    fill_array(array_a, secret[0]);
    fill_array(array_b, secret[1]);
    fill_array(array_c, secret[2]);
    dash::barrier();
  
    // Set option
  
    OutputStream os(_filename);
    os << dio::dataset("array_a")
       << array_a
       << dio::dataset("g1/array_b")
       << array_b
       << dio::dataset("g1/g2/array_c")
       << array_c;
  
    dash::barrier();
  }
  dash::Array<double> array_a;
  dash::Array<double> array_b;
  dash::Array<double> array_c;
  InputStream is(_filename);
  is >> dio::dataset("array_a")
     >> array_a
     >> dio::dataset("g1/array_b")
     >> array_b
     >> dio::dataset("g1/g2/array_c")
     >> array_c;
  
  dash::barrier();
  
  // Verify data
  verify_array(array_a, secret[0]);
  verify_array(array_b, secret[1]);
  verify_array(array_c, secret[2]);
}
#endif // DASH_ENABLE_HDF5



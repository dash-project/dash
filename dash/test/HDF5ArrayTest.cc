#ifdef DASH_ENABLE_HDF5

#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "HDF5ArrayTest.h"

#include "limits.h"

typedef int value_t;
typedef dash::Array<value_t, long> array_t;
namespace dio = dash::io;

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
    array[i] = coords[0] + secret;
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

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << arr1;

    dash::barrier();
  }
  DASH_LOG_DEBUG("Array successfully written ");

  // Create second array
  array_t arr2;
  dash::barrier();

#if 1	
	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset)
	   >> arr2;
#else
	dash::io::StoreHDF::read(arr2, _filename, _dataset);
#endif

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

		dio::HDF5OutputStream os(_filename);
    os << dio::HDF5store_pattern(false)
       << dio::HDF5dataset(_dataset)
			 << array_a;

    dash::barrier();
  }
  dash::Array<int> array_b;

	dio::HDF5InputStream is(_filename);
	is >> dio::HDF5dataset(_dataset)
     >> array_b;

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
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.store_pattern = false;
		
		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << dio::HDF5store_pattern(false)
       << array_a;

    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);

	dio::HDF5InputStream is(_filename);
 	is >> dio::HDF5dataset(_dataset)
     >> array_b;

  dash::barrier();

  // Verify
  verify_array(array_b, dash::myid());
}

TEST_F(HDF5ArrayTest, UnderfilledPattern)
{
  int   ext_x = dash::size() * 5 + 1;
  long  tilesize;
  {
    auto array_a = dash::Array<int>(ext_x);
    tilesize = array_a.pattern().blocksize(0);
    // Fill
    fill_array(array_a);
    dash::barrier();

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << dio::HDF5store_pattern(false)
       << array_a;

    dash::barrier();
  }
  dash::Array<int> array_b;
	dio::HDF5InputStream is(_filename);
 	is >> dio::HDF5dataset(_dataset)
     >> array_b;
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

		dio::HDF5OutputStream os(_filename);
		os << dio::HDF5dataset(_dataset)
       << dio::HDF5store_pattern(false)
       << array_a;

    dash::barrier();
  }
  auto array_b = dash::Array<int>(ext_x);
	dio::HDF5InputStream is(_filename);
 	is >> dio::HDF5dataset(_dataset)
     >> array_b;
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
    auto fopts = dash::io::StoreHDF::get_default_options();
    fopts.overwrite_file = false;

		{
			dio::HDF5OutputStream os(_filename);
			os << dio::HDF5dataset(_dataset)
    	   << array_a;
		}
		dio::HDF5OutputStream os(_filename, dio::HDF5FileOptions::Append);
		os << dio::HDF5dataset("datasettwo")
       << array_b;
	
    dash::barrier();
  }
  dash::Array<int>    array_c;
	dash::Array<double> array_d;

	dio::HDF5InputStream is(_filename);
 	is >> dio::HDF5dataset(_dataset)
     >> array_c
		 >> dio::HDF5dataset("datasettwo")
		 >> array_d;
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

		{
			dio::HDF5OutputStream os(_filename);
			os << dio::HDF5dataset(_dataset)
       	 << array_a;
		}
		dio::HDF5OutputStream os(_filename, dio::HDF5FileOptions::Append);
		os << dio::HDF5dataset(_dataset)
			 << array_a
			 << dio::HDF5modify_dataset()
			 << array_b;
	
    dash::barrier();
  }
  dash::Array<double>    array_c;
	dio::HDF5InputStream is(_filename);
 	is >> dio::HDF5dataset(_dataset)
     >> array_c;
	
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

		dash::io::HDF5OutputStream os(_filename, dash::io::HDF5FileOptions::Append);
		os << dash::io::HDF5dataset("settwo")
			 << dash::io::HDF5setpattern_key("custom_dash_pattern")
       << dash::io::HDF5store_pattern()
			 << array_a
			 << dash::io::HDF5modify_dataset()
			 << array_a;

		dash::barrier();
  }
  dash::Array<double>    array_b;
	dash::io::HDF5InputStream is(_filename);
	is >> dash::io::HDF5dataset("settwo")
		 >> dash::io::HDF5setpattern_key("custom_dash_pattern")
     >> dash::io::HDF5restore_pattern()
     >> array_b;
	
  dash::barrier();

  // Verify data
  verify_array(array_b, secret);
}


#endif // DASH_ENABLE_HDF5


